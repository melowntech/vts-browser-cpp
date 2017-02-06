#include "mesh.h"


// based on commit ec857179958e353433a906c16276258b89dd1f6a


struct pointer
{
    pointer(void *data = nullptr) : asVoid(data) {}

    union
    {
        void *asVoid;
        quint8 *asUint8;
        quint16 *asUint16;
        quint32 *asUint32;
        quint64 *asUint64;
        qint8 *asSint8;
        qint16 *asSint16;
        qint32 *asSint32;
        qint64 *asSint64;
        float *asFloat;
        double *asDouble;
    };
};


SubMesh::SubMesh(void *&streamData, quint16 version)
{
    pointer p(streamData);

    flags = *p.asSint8++;

    surfaceReference = 0;
    if (version > 1)
        surfaceReference = *p.asUint8++;

    textureLayer = *p.asUint16++;

    for (int i = 0; i < 3; i++)
        boxMin[i] = *p.asDouble++;
    for (int i = 0; i < 3; i++)
        boxMax[i] = *p.asDouble++;

    switch (version)
    {
    case 2: parseVersion2(p.asVoid); break;
    case 3: parseVersion3(p.asVoid); break;
    default: throw "unsupported mesh format";
    }
    streamData = p.asVoid;
}

inline const QVector3D parseVertexPosition(quint16 pos[3])
{
    QVector3D res;
    for (int i = 0; i < 3; i++)
        res[i] = pos[i] / 65535.f;
    return res;
}

inline const QVector2D parseVertexUv(quint16 pos[2])
{
    QVector2D res;
    for (int i = 0; i < 2; i++)
        res[i] = pos[i] / 65535.f;
    return res;
}

void SubMesh::parseVersion2(void *&streamData)
{
    pointer p(streamData);

    quint16 numVertices = *p.asUint16++;

    std::vector<QVector3D> vertices;
    std::vector<QVector2D> uvs;

    vertices.reserve(numVertices);

    if (flags & (1 << 1)) // external uvs
    {
        uvs.reserve(numVertices);

        struct Vertex
        {
            quint16 pos[3];
            quint16 uv[2];
        };

        Vertex *data = (Vertex*)p.asVoid;
        p.asUint8 += sizeof(Vertex) * numVertices;

        for (quint32 i = 0; i < numVertices; i++)
        {
            vertices.push_back(parseVertexPosition(data->pos));
            uvs.push_back(parseVertexUv(data->uv));
            data++;
        }
    }
    else
    {
        struct Vertex
        {
            quint16 pos[3];
        };

        Vertex *data = (Vertex*)p.asVoid;
        p.asUint8 += sizeof(Vertex) * numVertices;

        for (quint32 i = 0; i < numVertices; i++)
        {
            vertices.push_back(parseVertexPosition(data->pos));
            data++;
        }
    }

    if (flags & (1 << 0)) // internal uvs
    {

        quint16 numUvs = *p.asUint16++;

        struct Vertex
        {
            quint16 uvs[2];
        };

        Vertex *data = (Vertex*)p.asVoid;
        p.asUint8 += sizeof(Vertex) * numUvs;

        for (quint32 i = 0; i < numUvs; i++)
        {
            uvs.push_back(parseVertexUv(data->uvs));
            data++;
        }
    }

    quint16 numFaces = *p.asUint16++;
    faces.reserve(numFaces);

    if (flags & (1 << 0)) // internal uvs
    {
        struct Vertex
        {
            quint16 pos[3];
            quint16 uvs[3];
        };

        Vertex *data = (Vertex*)p.asVoid;
        p.asUint8 += sizeof(Vertex) * numFaces;

        Face f;
        for (quint16 i = 0; i < numFaces; i++)
        {
            Vertex d = *data++;
            for (int j = 0; j < 3; j++)
            {
                f.vertices[j].pos = vertices[d.pos[j]];
                f.vertices[j].uvs = uvs[d.uvs[j]];
            }
            faces.push_back(f);
        }
    }
    else
    {
        struct Vertex
        {
            quint16 pos[3];
        };

        Vertex *data = (Vertex*)p.asVoid;
        p.asUint8 += sizeof(Vertex) * numFaces;

        Face f;
        if (flags & (1 << 1)) // external uvs
        {
            for (quint16 i = 0; i < numFaces; i++)
            {
                Vertex d = *data++;
                for (int j = 0; j < 3; j++)
                {
                    f.vertices[j].pos = vertices[d.pos[j]];
                    f.vertices[j].uvs = uvs[d.pos[j]];
                }
                faces.push_back(f);
            }
        }
        else
        {
            for (quint16 i = 0; i < numFaces; i++)
            {
                Vertex d = *data++;
                for (int j = 0; j < 3; j++)
                    f.vertices[j].pos = vertices[d.pos[j]];
                faces.push_back(f);
            }
        }
    }

    streamData = p.asVoid;
} // end version 2

inline float parseDelta(pointer &p)
{
    qint32 v = *p.asUint8++;
    if (v & 0x80)
        v = (v & 0x7f) | (*p.asUint8++ << 7);
    if (v & 1)
        return -((v >> 1) + 1);
    return v >> 1;
}

inline quint16 parseWord(pointer &p)
{
    qint32 v = *p.asUint8++;
    if (v & 0x80)
        return (v & 0x7f) | (*p.asUint8++ << 7);
    return v;
}

void SubMesh::parseVersion3(void *&streamData)
{
    pointer p(streamData);

    quint16 numVertices = *p.asUint16++;
    quint16 quant = *p.asUint16++;

    QVector3D center = (boxMin + boxMax) * 0.5;
    QVector3D ext = boxMax - boxMin;
    float scale = std::max(ext[0], std::max(ext[1], ext[2]));
    QVector3D s = QVector3D(1, 1, 1) / ext;
    float multiplier = scale / quant;
    QVector3D shift = center - boxMin;

    std::vector<QVector3D> vertices;
    std::vector<QVector2D> uvs;

    vertices.reserve(numVertices);

    float x = 0, y = 0, z = 0;
    for (int i = 0; i < numVertices; i++)
    {
        x += parseDelta(p);
        y += parseDelta(p);
        z += parseDelta(p);
        QVector3D pos(x, y, z);
        vertices.push_back((pos * multiplier + shift) * s);
    }

    if (flags & (1 << 1)) // external uvs
    {
        uvs.reserve(numVertices);
        quant = *p.asUint16++;
        multiplier = 1.f / quant;

        x = 0, y = 0;
        for (int i = 0; i < numVertices; i++)
        {
            x += parseDelta(p);
            y += parseDelta(p);
            uvs.push_back(QVector2D(x, y) * multiplier);
        }
    }

    if (flags & (1 << 0)) // internal uvs
    {
        quint16 numUvs = *p.asUint16++;
        uvs.reserve(numUvs);
        quint16 quantU = *p.asUint16++;
        quint16 quantV = *p.asUint16++;
        QVector2D multiplier = QVector2D(1, 1) / QVector2D(quantU, quantV);

        x = 0, y = 0;
        for (int i = 0; i < numUvs; i++)
        {
            x += parseDelta(p);
            y += parseDelta(p);
            uvs.push_back(QVector2D(x, y) * multiplier);
        }
    }

    quint16 numFaces = *p.asUint16++;
    faces.resize(numFaces);

    if (flags & (1 << 1)) // external uvs
    {
        quint16 high = 0;
        for (quint16 i = 0; i < numFaces; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                quint16 d = parseWord(p);
                quint16 idx = high - d;
                if (!d)
                    high++;
                faces[i].vertices[j].pos = vertices[idx];
                faces[i].vertices[j].uvs = uvs[idx];
            }
        }
    }
    else
    {
        if (flags & (1 << 0)) // internal uvs
        {
            quint16 high = 0;
            for (quint16 i = 0; i < numFaces; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    quint16 d = parseWord(p);
                    quint16 idx = high - d;
                    if (!d)
                        high++;
                    faces[i].vertices[j].pos = vertices[idx];
                }
            }

            high = 0;
            for (quint16 i = 0; i < numFaces; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    quint16 d = parseWord(p);
                    quint16 idx = high - d;
                    if (!d)
                        high++;
                    faces[i].vertices[j].uvs = uvs[idx];
                }
            }
        }
        else
        {
            quint16 high = 0;
            for (quint16 i = 0; i < numFaces; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    quint16 d = parseWord(p);
                    quint16 idx = high - d;
                    if (!d)
                        high++;
                    faces[i].vertices[j].pos = vertices[idx];
                }
            }
        }
    }



    streamData = p.asVoid;
} // end version 3

Mesh::Mesh(void *streamData)
{

    pointer p(streamData);

    {
        qint8 a = *p.asSint8++;
        qint8 b = *p.asSint8++;
        if (a != 'M' || b != 'E')
            throw "invalid mesh structure";
    }

    version = *p.asUint16++;
    if (version < 1 || version > 3)
        throw "invalid mesh version";

    meanUndulation = *p.asDouble++;

    quint16 numMeshes = *p.asUint16++;
    subMeshes.reserve(numMeshes);
    for (quint16 i = 0; i < numMeshes; i++)
        subMeshes.push_back(new SubMesh(p.asVoid, version));
}


