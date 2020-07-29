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

#include "../utilities/obj.hpp"
#include "../gpuResource.hpp"
#include "../fetchTask.hpp"
#include "../map.hpp"

#include <dbglog/dbglog.hpp>
#include <vts-libs/vts/mesh.hpp>
#include <vts-libs/vts/meshio.hpp>

#include <optick.h>

namespace vts
{

GpuMeshSpec::GpuMeshSpec(const Buffer &buffer) :
    verticesCount(0), indicesCount(0),
    faceMode(FaceMode::Triangles), indexMode(GpuTypeEnum::UnsignedShort)
{
    OPTICK_EVENT("decode obj");

    uint32 dummy;
    uint32 fm;
    decodeObj(buffer, fm, vertices, indices, verticesCount, dummy);
    switch (fm)
    {
    case 1:
        faceMode = FaceMode::Points;
        break;
    case 2:
        faceMode = FaceMode::Lines;
        break;
    case 3:
        faceMode = FaceMode::Triangles;
        break;
    default:
        LOGTHROW(err2, std::invalid_argument) << "Invalid face mode";
    }
}

GpuMesh::GpuMesh(MapImpl *map, const std::string &name) :
    Resource(map, name)
{}

GpuMesh::GpuMesh(MapImpl *map, const std::string &name,
                 const vtslibs::vts::SubMesh &m) :
    Resource(map, name)
{
    OPTICK_EVENT("decode submesh");

    // this type of mesh is never managed by the resource manager
    // instead it is always owned by an aggregate mesh
    state = Resource::State::errorFatal;

    assert(m.facesTc.size() == m.faces.size() || m.facesTc.empty());
    assert(m.etc.size() == m.vertices.size() || m.etc.empty());

    uint32 vertexSize = sizeof(vec3f);
    if (m.tc.size())
        vertexSize += sizeof(vec2ui16);
    if (m.etc.size())
        vertexSize += sizeof(vec2ui16);

    GpuMeshSpec spec;

#if 1 // indexed mesh

    { // vertex attributes
        uint32 offset = 0;

        { // positions
            spec.attributes[0].enable = true;
            spec.attributes[0].components = 3;
            spec.attributes[0].offset = offset;
            spec.attributes[0].stride = vertexSize;
            offset += sizeof(vec3f);
        }

        if (!m.tc.empty())
        { // internal uv
            spec.attributes[1].enable = true;
            spec.attributes[1].type = GpuTypeEnum::UnsignedShort;
            spec.attributes[1].components = 2;
            spec.attributes[1].normalized = true;
            spec.attributes[1].offset = offset;
            spec.attributes[1].stride = vertexSize;
            offset += sizeof(vec2ui16);
        }

        if (!m.etc.empty())
        { // external uv
            spec.attributes[2].enable = true;
            spec.attributes[2].type = GpuTypeEnum::UnsignedShort;
            spec.attributes[2].components = 2;
            spec.attributes[2].normalized = true;
            spec.attributes[2].offset = offset;
            spec.attributes[2].stride = vertexSize;
            offset += sizeof(vec2ui16);
        }

        assert(offset == vertexSize);
    }

    if (m.tc.empty())
    {
        // external control
        spec.verticesCount = m.vertices.size();
        spec.vertices.allocate(spec.verticesCount * vertexSize);
        spec.indicesCount = m.faces.size() * 3;
        spec.indices.allocate(spec.indicesCount * sizeof(uint16));

        { // indices
            uint16 *io = (uint16*)spec.indices.data();
            for (const auto &it : m.faces)
            {
                for (uint32 j = 0; j < 3; j++)
                    *io++ = it[j];
            }
            assert((char*)io == spec.indices.dataEnd());
        }

        { // positions
            vec3f *o = (vec3f*)(spec.vertices.data()
                                + spec.attributes[0].offset);
            for (const auto &it : m.vertices)
            {
                vec3 v = vecFromUblas<vec3>(it);
                *o = v.cast<float>();
                o = (vec3f*)((char*)o + vertexSize);
            }
        }

        if (spec.attributes[2].enable)
        { // external uvs
            vec2ui16 *o = (vec2ui16*)(spec.vertices.data()
                                      + spec.attributes[2].offset);
            for (const auto &it : m.etc)
            {
                *o = vec2to2ui16(vecFromUblas<vec2f>(it));
                o = (vec2ui16*)((char*)o + vertexSize);
            }
        }
    }
    else
    {
        // internal control
        spec.verticesCount = m.tc.size();
        spec.vertices.allocate(spec.verticesCount * vertexSize);
        spec.indicesCount = m.facesTc.size() * 3;
        spec.indices.allocate(spec.indicesCount * sizeof(uint16));

        { // indices
            uint16 *io = (uint16*)spec.indices.data();
            for (const auto &it : m.facesTc)
            {
                for (uint32 j = 0; j < 3; j++)
                    *io++ = it[j];
            }
            assert((char*)io == spec.indices.dataEnd());
        }

        // vertex data
        char *ps = spec.vertices.data() + spec.attributes[0].offset;
        char *is = spec.vertices.data() + spec.attributes[1].offset;
        char *es = spec.vertices.data() + spec.attributes[2].offset;
        for (uint32 fi = 0, fc = m.facesTc.size(); fi != fc; fi++)
        {
            for (uint32 vi = 0; vi < 3; vi++)
            {
                uint32 oi = m.facesTc[fi][vi];
                assert(oi < spec.verticesCount);
                uint32 ii = m.faces[fi][vi];
                assert(ii < m.vertices.size());
                { // position
                    vec3 v = vecFromUblas<vec3>(m.vertices[ii]);
                    *(vec3f*)(ps + oi * vertexSize) = v.cast<float>();
                }
                { // internal uv
                    vec2ui16 uv = vec2to2ui16(vecFromUblas<vec2f>(m.tc[oi]));
                    *(vec2ui16*)(is + oi * vertexSize) = uv;
                }
                if (spec.attributes[2].enable)
                { // external uv
                    vec2ui16 uv = vec2to2ui16(vecFromUblas<vec2f>(m.etc[ii]));
                    *(vec2ui16*)(es + oi * vertexSize) = uv;
                }
            }
        }
    }

    faces = spec.indicesCount / 3;

#else // indexed

    spec.verticesCount = m.faces.size() * 3;
    spec.vertices.allocate(spec.verticesCount * vertexSize);
    uint32 offset = 0;

    { // vertices
        spec.attributes[0].enable = true;
        spec.attributes[0].components = 3;
        spec.attributes[0].offset = offset;
        spec.attributes[0].stride = vertexSize;
        vec3f *b = (vec3f*)spec.vertices.data();
        for (vtslibs::vts::Point3u32 f : m.faces)
        {
            for (uint32 j = 0; j < 3; j++)
            {
                vec3 p3 = vecFromUblas<vec3>(m.vertices[f[j]]);
                *b = p3.cast<float>();
                b = (vec3f*)((char*)b + vertexSize);
            }
        }
        offset += sizeof(vec3f);
    }

    if (!m.tc.empty())
    { // internal, separated
        spec.attributes[1].enable = true;
        spec.attributes[1].type = GpuTypeEnum::UnsignedShort;
        spec.attributes[1].components = 2;
        spec.attributes[1].normalized = true;
        spec.attributes[1].offset = offset;
        spec.attributes[1].stride = vertexSize;
        vec2ui16 *b = (vec2ui16*)(spec.vertices.data() + offset);
        for (vtslibs::vts::Point3u32 f : m.facesTc)
        {
            for (uint32 j = 0; j < 3; j++)
            {
                *b = vec2to2ui16(vecFromUblas<vec2f>(m.tc[f[j]]));
                b = (vec2ui16*)((char*)b + vertexSize);
            }
        }
        offset += sizeof(vec2ui16);
    }

    if (!m.etc.empty())
    { // external, interleaved
        spec.attributes[2].enable = true;
        spec.attributes[2].type = GpuTypeEnum::UnsignedShort;
        spec.attributes[2].components = 2;
        spec.attributes[2].normalized = true;
        spec.attributes[2].offset = offset;
        spec.attributes[2].stride = vertexSize;
        vec2ui16 *b = (vec2ui16*)(spec.vertices.data() + offset);
        for (vtslibs::vts::Point3u32 f : m.faces)
        {
            for (uint32 j = 0; j < 3; j++)
            {
                *b = vec2to2ui16(vecFromUblas<vec2f>(m.etc[f[j]]));
                b = (vec2ui16*)((char*)b + vertexSize);
            }
        }
        offset += sizeof(vec2ui16);
    }

    faces = spec.verticesCount / 3;

#endif // indexed

    decodeData = std::make_shared<GpuMeshSpec>(std::move(spec));
}

void GpuMesh::decode()
{
    LOG(info1) << "Decoding (gpu) mesh '" << name << "'";
    OPTICK_EVENT("decode gpu mesh");
    std::shared_ptr<GpuMeshSpec> spec
        = std::make_shared<GpuMeshSpec>(fetch->reply.content);
    spec->attributes[0].enable = true;
    spec->attributes[0].stride = sizeof(vec3f) + sizeof(vec2f);
    spec->attributes[0].components = 3;
    spec->attributes[1].enable = true;
    spec->attributes[1].stride = spec->attributes[0].stride;
    spec->attributes[1].components = 2;
    spec->attributes[1].offset = sizeof(vec3f);
    spec->attributes[2] = spec->attributes[1];
    decodeData = std::static_pointer_cast<void>(spec);
}

void GpuMesh::upload()
{
    LOG(info1) << "Uploading (gpu) mesh '" << name << "'";
    auto spec = std::static_pointer_cast<GpuMeshSpec>(decodeData);
    map->callbacks.loadMesh(info, *spec, name);
    info.ramMemoryCost += sizeof(*this);
}

FetchTask::ResourceType GpuMesh::resourceType() const
{
    return FetchTask::ResourceType::Undefined;
}

namespace
{

const mat4 findNormToPhys(const math::Extents3 &extents)
{
    vec3 u = vecFromUblas<vec3>(extents.ur);
    vec3 l = vecFromUblas<vec3>(extents.ll);
    vec3 d = (u - l) * 0.5;
    vec3 c = (u + l) * 0.5;
    mat4 sc = scaleMatrix(d(0), d(1), d(2));
    mat4 tr = translationMatrix(c);
    return tr * sc;
}

} // namespace

MeshAggregate::MeshAggregate(MapImpl *map, const std::string &name) :
    Resource(map, name)
{}

void MeshAggregate::decode()
{
    LOG(info2) << "Decoding (aggregated) mesh <" << name << ">";
    OPTICK_EVENT("decode aggregated mesh");

    detail::BufferStream w(fetch->reply.content);
    vtslibs::vts::NormalizedSubMesh::list meshes = vtslibs::vts::
            loadMeshProperNormalized(w, name);

    submeshes.clear();
    submeshes.reserve(meshes.size());

    for (uint32 mi = 0, me = meshes.size(); mi != me; mi++)
    {
        const vtslibs::vts::SubMesh &m = meshes[mi].submesh;

        std::stringstream ss;
        ss << name << "#" << mi;
        std::shared_ptr<GpuMesh> gm
            = std::make_shared<GpuMesh>(map, ss.str(), m);

        const auto &spec = *std::static_pointer_cast
                <GpuMeshSpec>(gm->decodeData);
        MeshPart part;
        part.renderable = gm;
        part.normToPhys = findNormToPhys(meshes[mi].extents)
                * scaleMatrix(map->options.renderTilesScale);
        part.internalUv = spec.attributes[1].enable;
        part.externalUv = spec.attributes[2].enable;
        part.textureLayer = m.textureLayer ? *m.textureLayer : 0;
        part.surfaceReference = m.surfaceReference;
        submeshes.push_back(part);

#ifndef __EMSCRIPTEN__
        if (map->options.debugExtractRawResources)
        {
            static const std::string prefix = "extracted/";
            std::string b, c;
            std::string path = convertNameToFolderAndFile(this->name, b, c);
            std::stringstream ss;
            ss << prefix << path << "_" << mi << ".obj";
            path = ss.str();
            if (!boost::filesystem::exists(path))
            {
                boost::filesystem::create_directories(prefix + b);
                vtslibs::vts::SubMesh msh(m);

                // denormalize vertex positions
                for (auto &v : msh.vertices)
                {
                    v = vecToUblas<math::Point3>(
                        vec4to3(vec4(part.normToPhys
                            * vec3to4(vecFromUblas<vec3>(v), 1))));
                }

                // copy external uv if there are no internal uv
                if (msh.facesTc.empty() && !msh.etc.empty())
                {
                    msh.facesTc = msh.faces;
                    msh.etc.swap(msh.tc);
                }

                // save to stream (with increased precision)
                std::ofstream f;
                f.exceptions(std::ios::badbit | std::ios::failbit);
                f.open(path, std::ios_base::out | std::ios_base::trunc);
                f.precision(20);
                vtslibs::vts::saveSubMeshAsObj(f, msh, mi);
            }
        }
#endif // emscripten
    }
}

void MeshAggregate::upload()
{
    LOG(info2) << "Uploading (aggregated) mesh <" << name << ">";

    info.ramMemoryCost += sizeof(*this) + submeshes.size() * sizeof(MeshPart);
    for (const auto &it : submeshes)
    {
        it.renderable->upload();
        info.gpuMemoryCost += it.renderable->info.gpuMemoryCost;
        info.ramMemoryCost += it.renderable->info.ramMemoryCost;
        it.renderable->decodeData.reset();
        it.renderable->state = Resource::State::ready;
    }
}

FetchTask::ResourceType MeshAggregate::resourceType() const
{
    return FetchTask::ResourceType::Mesh;
}

} // namespace vts
