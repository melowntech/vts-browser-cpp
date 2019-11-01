/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "geodata.hpp"

namespace vts { namespace renderer
{

namespace
{

void makeTextureMoreSquare(GpuTextureSpec &spec)
{
    assert(spec.height == 2);
    assert(spec.components == 3);
    assert(spec.type == GpuTypeEnum::Float);
    if (spec.width < 10)
        return;
    uint32 w = std::sqrt(spec.width * 2) + 1;
    w = (w / 4) * 4;
    uint32 h = spec.width / w * 2;
    if (w * h < spec.width * 2)
        h += 2;
    assert(w * h >= spec.width * 2);
    Buffer b;
    b.allocate(w * h * sizeof(vec3f));
    b.zero();
    vec3f *inp = (vec3f*)spec.buffer.data();
    vec3f *inn = inp + spec.width;
    vec3f *out = (vec3f*)b.data();
    uint32 mm = b.size() / sizeof(vec3f);
    (void)mm;
    for (uint32 i = 0; i < spec.width; i++)
    {
        uint32 yy = i / w;
        uint32 xx = i % w;
        uint32 pi = (yy * 2 + 0) * w + xx;
        uint32 ni = (yy * 2 + 1) * w + xx;
        assert(pi < mm);
        assert(ni < mm);
        out[pi] = *inp++;
        out[ni] = *inn++;
    }
    std::swap(spec.buffer, b);
    spec.width = w;
    spec.height = h;
}

float oneMeterInModel(const mat4 &model, const mat4 &modelInv)
{
    vec4 a = model * vec4(0, 0, 0, 1);
    vec4 b = a + vec4(1, 0, 0, 0); // add one meter in world space
    vec4 c = modelInv * b;
    return length(vec4to3(c)); // measure the change in model space
}

} // namespace

void GeodataTile::loadLines()
{
    uint32 totalPoints = getTotalPoints(); // example: 7
    uint32 linesCount = spec.positions.size(); // 2
    uint32 segmentsCount = totalPoints - linesCount; // 5
    uint32 jointsCount = segmentsCount - linesCount; // 3
    uint32 capsCount = linesCount * 2; // 4
    uint32 trianglesCount = (segmentsCount + jointsCount + capsCount) * 2; // 24
    uint32 indicesCount = trianglesCount * 3; // 72
    // point index = (vertex index / 4 + vertex index % 2)
    // corner = vertex index % 4

    Buffer texBuffer;
    Buffer indBuffer;

    // prepare texture buffer and mesh indices
    {
        texBuffer.resize(totalPoints * sizeof(vec3f) * 2);
        vec3f *bufPos = (vec3f*)texBuffer.data();
        vec3f *bufUps = bufPos + totalPoints;
        vec3f *texBufHalf = bufUps;
        (void)texBufHalf;

        indBuffer.resize(indicesCount * sizeof(uint32));
        uint32 *bufInd = (uint32*)indBuffer.data();
        uint32 *capsInd = bufInd + indicesCount - capsCount * 6;
        uint32 *capsStart = capsInd;
        (void)capsStart;

        uint32 current = 0;
        for (uint32 li = 0; li < linesCount; li++)
        {
            uint32 first = bufPos - (vec3f*)texBuffer.data();
            const auto &points = spec.positions[li];
            uint32 pointsCount = points.size();
            for (uint32 pi = 0; pi < pointsCount; pi++)
            {
                vec3f p = rawToVec3(points[pi].data());
                vec3f u = modelUp(p);
                *bufPos++ = p;
                *bufUps++ = u;
                // add joint
                if (pi > 1)
                {
                    *bufInd++ = current + 1 - 4;
                    *bufInd++ = current + 4 - 4;
                    *bufInd++ = current + 6 - 4;
                    *bufInd++ = current + 1 - 4;
                    *bufInd++ = current + 6 - 4;
                    *bufInd++ = current + 3 - 4;
                }
                // add segment
                if (pi > 0)
                {
                    *bufInd++ = current + 0;
                    *bufInd++ = current + 1;
                    *bufInd++ = current + 3;
                    *bufInd++ = current + 0;
                    *bufInd++ = current + 3;
                    *bufInd++ = current + 2;
                    current += 4;
                }
            }
            // make a gap
            current += 4;
            // caps
            {
                uint32 last = first + pointsCount - 2;
                *capsInd++ = (1 << 30) + first * 4 + 0;
                *capsInd++ = (1 << 30) + first * 4 + 3;
                *capsInd++ = (1 << 30) + first * 4 + 1;
                *capsInd++ = (1 << 30) + first * 4 + 0;
                *capsInd++ = (1 << 30) + first * 4 + 2;
                *capsInd++ = (1 << 30) + first * 4 + 3;
                *capsInd++ = (3 << 29) + last * 4 + 0;
                *capsInd++ = (3 << 29) + last * 4 + 3;
                *capsInd++ = (3 << 29) + last * 4 + 1;
                *capsInd++ = (3 << 29) + last * 4 + 0;
                *capsInd++ = (3 << 29) + last * 4 + 2;
                *capsInd++ = (3 << 29) + last * 4 + 3;
                // highest (sign) bit = unused
                // second highest bit = is cap
                // third highest bit = is end cap
            }
        }

        assert(bufPos == texBufHalf);
        assert(bufUps == (vec3f*)texBuffer.dataEnd());
        assert(bufInd == capsStart);
        assert(capsInd == (uint32*)indBuffer.dataEnd());
    }

    // prepare the texture
    {
        GpuTextureSpec tex;
        tex.buffer = std::move(texBuffer);
        tex.width = totalPoints;
        tex.height = 2;
        tex.components = 3;
        tex.type = GpuTypeEnum::Float;
        tex.filterMode = GpuTextureSpec::FilterMode::Nearest;
        tex.wrapMode = GpuTextureSpec::WrapMode::ClampToEdge;
        makeTextureMoreSquare(tex);
        ResourceInfo ri;
        renderer->api->loadTexture(ri, tex, debugId);
        this->texture = std::static_pointer_cast<Texture>(ri.userData);
        addMemory(ri);
    }

    // prepare the mesh
    {
        GpuMeshSpec msh;
        msh.faceMode = GpuMeshSpec::FaceMode::Triangles;
        msh.indices = std::move(indBuffer);
        msh.indicesCount = indicesCount;
        msh.indexMode = GpuTypeEnum::UnsignedInt;
        ResourceInfo ri;
        renderer->api->loadMesh(ri, msh, debugId);
        this->mesh = std::static_pointer_cast<Mesh>(ri.userData);
        addMemory(ri);
    }

    // prepare UBO
    {
        struct UboLineData
        {
            vec4f color;
            vec4f visibilities;
            vec4f uniUnitsRadius;
        };
        UboLineData uboLineData;

        uboLineData.color = rawToVec4(spec.unionData.line.color);
        uboLineData.visibilities
            = rawToVec4(spec.commonData.visibilities);
        uboLineData.uniUnitsRadius = vec4f(
            (float)spec.unionData.line.units,
            spec.unionData.line.width * 0.5f, 0.f, 0.f);
        if (spec.type == GpuGeodataSpec::Type::LineFlat)
            uboLineData.uniUnitsRadius[1]
                *= oneMeterInModel(model, modelInv);

        uniform = std::make_unique<UniformBuffer>();
        uniform->setDebugId(debugId);
        uniform->bind();
        uniform->load(uboLineData, GL_STATIC_DRAW);
        info->gpuMemoryCost += sizeof(uboLineData);
    }
}

void GeodataTile::loadPoints()
{
    uint32 totalPoints = getTotalPoints(); // example: 7
    uint32 trianglesCount = totalPoints * 2; // 14
    uint32 indicesCount = trianglesCount * 3; // 42
    // point index = vertex index / 4
    // corner = vertex index % 4

    Buffer texBuffer;
    Buffer indBuffer;

    // prepare texture buffer and mesh indices
    {
        texBuffer.resize(totalPoints * sizeof(vec3f) * 2);
        vec3f *bufPos = (vec3f*)texBuffer.data();
        vec3f *bufUps = (vec3f*)texBuffer.data() + totalPoints;
        vec3f *texBufHalf = bufUps;
        (void)texBufHalf;

        indBuffer.resize(indicesCount * sizeof(uint32));
        uint32 *bufInd = (uint32*)indBuffer.data();
        uint32 current = 0;

        assert(spec.positions.size() == totalPoints);
        for (uint32 pi = 0; pi < totalPoints; pi++)
        {
            vec3f p = rawToVec3(spec.positions[pi][0].data());
            vec3f u = modelUp(p);
            *bufPos++ = p;
            *bufUps++ = u;
            *bufInd++ = current + 0;
            *bufInd++ = current + 1;
            *bufInd++ = current + 3;
            *bufInd++ = current + 0;
            *bufInd++ = current + 3;
            *bufInd++ = current + 2;
            current += 4;
        }

        assert(bufPos == texBufHalf);
        assert(bufUps == (vec3f*)texBuffer.dataEnd());
        assert(bufInd == (uint32*)indBuffer.dataEnd());
    }

    // prepare the texture
    {
        GpuTextureSpec tex;
        tex.buffer = std::move(texBuffer);
        tex.width = totalPoints;
        tex.height = 2;
        tex.components = 3;
        tex.type = GpuTypeEnum::Float;
        tex.filterMode = GpuTextureSpec::FilterMode::Nearest;
        tex.wrapMode = GpuTextureSpec::WrapMode::ClampToEdge;
        makeTextureMoreSquare(tex);
        ResourceInfo ri;
        renderer->api->loadTexture(ri, tex, debugId);
        this->texture = std::static_pointer_cast<Texture>(ri.userData);
        addMemory(ri);
    }

    // prepare the mesh
    {
        GpuMeshSpec msh;
        msh.faceMode = GpuMeshSpec::FaceMode::Triangles;
        msh.indices = std::move(indBuffer);
        msh.indicesCount = indicesCount;
        msh.indexMode = GpuTypeEnum::UnsignedInt;
        ResourceInfo ri;
        renderer->api->loadMesh(ri, msh, debugId);
        this->mesh = std::static_pointer_cast<Mesh>(ri.userData);
        addMemory(ri);
    }

    // prepare UBO
    {
        struct UboPointData
        {
            vec4f color;
            vec4f visibilities;
            vec4f uniUnitsRadius;
        };
        UboPointData uboPointData;

        uboPointData.color = rawToVec4(spec.unionData.point.color);
        uboPointData.visibilities
            = rawToVec4(spec.commonData.visibilities);
        uboPointData.uniUnitsRadius = vec4f(
            (float)spec.unionData.point.units,
            spec.unionData.point.radius, 0.f, 0.f);
        if (spec.type == GpuGeodataSpec::Type::PointFlat)
            uboPointData.uniUnitsRadius[1]
                *= oneMeterInModel(model, modelInv);

        uniform = std::make_unique<UniformBuffer>();
        uniform->setDebugId(debugId);
        uniform->bind();
        uniform->load(uboPointData, GL_STATIC_DRAW);
        info->gpuMemoryCost += sizeof(uboPointData);
    }
}

void GeodataTile::loadIcons()
{
    assert(spec.iconCoords.size() == spec.positions.size());
    copyPoints();
    info->ramMemoryCost += points.size() * sizeof(decltype(points[0]));
}

void GeodataTile::loadTriangles()
{
    // prepare mesh
    {
        GpuMeshSpec msh;
        msh.faceMode = GpuMeshSpec::FaceMode::Triangles;
        msh.attributes[0].enable = true;
        msh.attributes[0].components = 3;
        msh.attributes[0].type = GpuTypeEnum::Float;
        msh.verticesCount = getTotalPoints();
        msh.vertices.allocate(sizeof(float) * 3 * msh.verticesCount);
        float *f = (float*)msh.vertices.data();
        for (const auto &it1 : spec.positions)
            for (const auto &it2 : it1)
                for (float it : it2)
                    *f++ = it;
        ResourceInfo ri;
        renderer->api->loadMesh(ri, msh, debugId);
        this->mesh = std::static_pointer_cast<Mesh>(ri.userData);
        addMemory(ri);
    }

    // prepare UBO
    {
        struct UboTriangleData
        {
            vec4f color;
            vec4f visibilities;
            sint32 shading;
        };
        UboTriangleData uboTriangleData;

        uboTriangleData.color = rawToVec4(spec.unionData.triangles.color);
        uboTriangleData.visibilities
            = rawToVec4(spec.commonData.visibilities);
        uboTriangleData.shading = (sint32)spec.unionData.triangles.style;

        uniform = std::make_unique<UniformBuffer>();
        uniform->setDebugId(debugId);
        uniform->bind();
        uniform->load(uboTriangleData, GL_STATIC_DRAW);
        info->gpuMemoryCost += sizeof(uboTriangleData);
    }
}

} } // namespace vts renderer priv

