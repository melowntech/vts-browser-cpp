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

void GeodataBase::prepareTextureForLinesAndPoints(Buffer &&texBuffer,
    uint32 totalPoints)
{
    GpuTextureSpec tex;
    tex.buffer = std::move(texBuffer);
    tex.width = totalPoints;
    tex.height = 2;
    tex.components = 3;
    tex.internalFormat = GL_RGB32F;
    tex.type = GpuTypeEnum::Float;
    tex.filterMode = GpuTextureSpec::FilterMode::Nearest;
    tex.wrapMode = GpuTextureSpec::WrapMode::ClampToEdge;
    ResourceInfo ri;
    renderer->api->loadTexture(ri, tex, debugId);
    this->texture = std::static_pointer_cast<Texture>(ri.userData);
    addMemory(ri);
}

void GeodataBase::prepareMeshForLinesAndPoints(Buffer &&indBuffer,
    uint32 indicesCount)
{
    GpuMeshSpec msh;
    msh.faceMode = GpuMeshSpec::FaceMode::Triangles;
    msh.indices = std::move(indBuffer);
    msh.indicesCount = indicesCount;
    ResourceInfo ri;
    renderer->api->loadMesh(ri, msh, debugId);
    this->mesh = std::static_pointer_cast<Mesh>(ri.userData);
    addMemory(ri);
}

void GeodataBase::loadLines()
{
    uint32 totalPoints = getTotalPoints(); // example: 7
    uint32 linesCount = spec.positions.size(); // 2
    uint32 segmentsCount = totalPoints - linesCount; // 5
    uint32 jointsCount = segmentsCount - linesCount; // 3
    uint32 trianglesCount = (segmentsCount + jointsCount) * 2; // 16
    uint32 indicesCount = trianglesCount * 3; // 48
    // point index = (vertex index / 4 + vertex index % 2)
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

        indBuffer.resize(indicesCount * sizeof(uint16));
        uint16 *bufInd = (uint16*)indBuffer.data();
        uint16 current = 0;

        for (uint32 li = 0; li < linesCount; li++)
        {
            const std::vector<std::array<float, 3>> &points
                = spec.positions[li];
            uint32 pointsCount = points.size();
            for (uint32 pi = 0; pi < pointsCount; pi++)
            {
                vec3f p = rawToVec3(points[pi].data());
                vec3f u = modelUp(p);
                *bufPos++ = p;
                *bufUps++ = u;
                if (pi > 1)
                { // add joint
                    *bufInd++ = current + 1 - 4;
                    *bufInd++ = current + 4 - 4;
                    *bufInd++ = current + 6 - 4;
                    *bufInd++ = current + 1 - 4;
                    *bufInd++ = current + 6 - 4;
                    *bufInd++ = current + 3 - 4;
                }
                if (pi > 0)
                { // add segment
                    *bufInd++ = current + 0;
                    *bufInd++ = current + 1;
                    *bufInd++ = current + 3;
                    *bufInd++ = current + 0;
                    *bufInd++ = current + 3;
                    *bufInd++ = current + 2;
                    current += 4;
                }
            }
            current += 4; // make a gap
        }

        assert(bufPos == texBufHalf);
        assert(bufUps == (vec3f*)texBuffer.dataEnd());
        assert(bufInd == (uint16*)indBuffer.dataEnd());
    }

    // prepare the texture
    prepareTextureForLinesAndPoints(std::move(texBuffer), totalPoints);

    // prepare the mesh
    prepareMeshForLinesAndPoints(std::move(indBuffer), indicesCount);

    // prepare UBO
    {
        struct UboLineData
        {
            vec4f color;
            vec4f visibilities;
            vec4f typePlusUnitsPlusWidth;
        };
        UboLineData uboLineData;

        uboLineData.color = rawToVec4(spec.unionData.line.color);
        uboLineData.visibilities
            = rawToVec4(spec.commonData.visibilities);
        uboLineData.typePlusUnitsPlusWidth
            = vec4f((float)spec.type, (float)spec.unionData.line.units,
                spec.unionData.line.width, 0.f);

        uniform = std::make_shared<UniformBuffer>();
        uniform->debugId = debugId;
        uniform->bind();
        uniform->load(uboLineData);
        info->gpuMemoryCost += sizeof(uboLineData);
    }
}

void GeodataBase::loadPoints()
{
    uint32 totalPoints = getTotalPoints();
    uint32 trianglesCount = totalPoints * 2;
    uint32 indicesCount = trianglesCount * 3;

    Buffer texBuffer;
    Buffer indBuffer;

    // prepare texture buffer and mesh indices
    {
        texBuffer.resize(totalPoints * sizeof(vec3f) * 2);
        vec3f *bufPos = (vec3f*)texBuffer.data();
        vec3f *bufUps = (vec3f*)texBuffer.data() + totalPoints;
        vec3f *texBufHalf = bufUps;
        (void)texBufHalf;

        indBuffer.resize(indicesCount * sizeof(uint16));
        uint16 *bufInd = (uint16*)indBuffer.data();
        uint16 current = 0;

        for (uint32 li = 0, lin = spec.positions.size();
            li < lin; li++)
        {
            const std::vector<std::array<float, 3>> &points
                = spec.positions[li];
            for (uint32 pi = 0, pin = points.size();
                pi < pin; pi++)
            {
                vec3f p = rawToVec3(points[pi].data());
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
        }

        assert(bufPos == texBufHalf);
        assert(bufUps == (vec3f*)texBuffer.dataEnd());
        assert(bufInd == (uint16*)indBuffer.dataEnd());
    }

    // prepare the texture
    prepareTextureForLinesAndPoints(std::move(texBuffer), totalPoints);

    // prepare the mesh
    prepareMeshForLinesAndPoints(std::move(indBuffer), indicesCount);

    // prepare UBO
    {
        struct UboPointData
        {
            vec4f color;
            vec4f visibilities;
            vec4f typePlusRadius;
        };
        UboPointData uboPointData;

        uboPointData.color = rawToVec4(spec.unionData.point.color);
        uboPointData.visibilities
            = rawToVec4(spec.commonData.visibilities);
        uboPointData.typePlusRadius
            = vec4f((float)spec.type,
                spec.unionData.point.radius * 2.f, // NDC space is twice as large
                0.f, 0.f);

        uniform = std::make_shared<UniformBuffer>();
        uniform->debugId = debugId;
        uniform->bind();
        uniform->load(uboPointData);
        info->gpuMemoryCost += sizeof(uboPointData);
    }
}

void GeodataBase::loadIcons()
{
    assert(spec.iconCoords.size() == spec.positions.size());
    for (uint32 i = 0, e = spec.iconCoords.size(); i != e; i++)
    {
        Point t;
        vec3f modelPosition = rawToVec3(spec.positions[i][0].data());
        t.worldPosition = vec4to3(vec4(rawToMat4(spec.model)
            * vec3to4(modelPosition, 1).cast<double>()));
        t.worldUp = worldUp(modelPosition);
        points.push_back(t);
    }
    info->ramMemoryCost += points.size() * sizeof(decltype(points[0]));
}

void GeodataBase::loadTriangles()
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

        uniform = std::make_shared<UniformBuffer>();
        uniform->debugId = debugId;
        uniform->bind();
        uniform->load(uboTriangleData);
        info->gpuMemoryCost += sizeof(uboTriangleData);
    }
}

} } // namespace vts renderer priv

