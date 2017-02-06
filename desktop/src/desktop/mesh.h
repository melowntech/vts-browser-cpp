#ifndef MESH_H
#define MESH_H

#include <QtGlobal>
#include <QVector3D>
#include <QVector2D>

#include <vector>

class SubMesh
{
public:
    SubMesh(void *&streamData, quint16 version);

    void parseVersion2(void *&streamData);
    void parseVersion3(void *&streamData);

    qint8 flags;
    quint8 surfaceReference;
    quint16 textureLayer;
    QVector3D boxMin, boxMax;

    struct Face
    {
        struct Vertex
        {
            QVector3D pos;
            QVector2D uvs;
        } vertices[3];
    };
    std::vector<Face> faces;
};

class Mesh
{
public:
    Mesh(void *streamData);

    quint16 version;
    double meanUndulation;

    std::vector<SubMesh*> subMeshes;
};

#endif // MESH_H
