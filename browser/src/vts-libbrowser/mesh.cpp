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

#include "map.hpp"
#include "obj.hpp"

namespace vts
{

GpuMeshSpec::GpuMeshSpec() : verticesCount(0), indicesCount(0),
    faceMode(FaceMode::Triangles)
{}

GpuMeshSpec::GpuMeshSpec(const Buffer &buffer) :
    verticesCount(0), indicesCount(0),
    faceMode(FaceMode::Triangles)
{
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

GpuMeshSpec::VertexAttribute::VertexAttribute() : offset(0), stride(0),
    components(0), type(Type::Float), enable(false), normalized(false)
{}

GpuMesh::GpuMesh(MapImpl *map, const std::string &name) :
    Resource(map, name, FetchTask::ResourceType::MeshPart)
{}

void GpuMesh::load()
{
    LOG(info1) << "Loading (gpu) mesh '" << name << "'";
    GpuMeshSpec spec(reply.content);
    spec.attributes.resize(3);
    spec.attributes[0].enable = true;
    spec.attributes[0].stride = sizeof(vec3f) + sizeof(vec2f);
    spec.attributes[0].components = 3;
    spec.attributes[1].enable = true;
    spec.attributes[1].stride = spec.attributes[0].stride;
    spec.attributes[1].components = 2;
    spec.attributes[1].offset = sizeof(vec3f);
    spec.attributes[2] = spec.attributes[1];
    map->callbacks.loadMesh(info, spec);
    info.ramMemoryCost += sizeof(*this);
}

MeshPart::MeshPart() :
    textureLayer(0), surfaceReference(0),
    internalUv(false), externalUv(false)
{}

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
    Resource(map, name, FetchTask::ResourceType::Mesh)
{}

void MeshAggregate::load()
{
    LOG(info2) << "Loading (aggregated) mesh <" << name << ">";
    
    detail::Wrapper w(reply.content);
    vtslibs::vts::NormalizedSubMesh::list meshes = vtslibs::vts::
            loadMeshProperNormalized(w, name);

    submeshes.clear();
    submeshes.reserve(meshes.size());

    for (uint32 mi = 0, me = meshes.size(); mi != me; mi++)
    {
        vtslibs::vts::SubMesh &m = meshes[mi].submesh;

        char tmp[10];
        sprintf(tmp, "%d", mi);
        std::shared_ptr<GpuMesh> gm = std::make_shared<GpuMesh>(map,
            name + "#" + tmp);

        uint32 vertexSize = sizeof(vec3f);
        if (m.tc.size())
            vertexSize += sizeof(vec2f);
        if (m.etc.size())
            vertexSize += sizeof(vec2f);

        GpuMeshSpec spec;
        spec.attributes.resize(3);
        spec.verticesCount = m.faces.size() * 3;
        spec.vertices.allocate(spec.verticesCount * vertexSize);
        uint32 offset = 0;

        { // vertices
            spec.attributes[0].enable = true;
            spec.attributes[0].components = 3;
            vec3f *b = (vec3f*)spec.vertices.data();
            for (vtslibs::vts::Point3u32 f : m.faces)
            {
                for (uint32 j = 0; j < 3; j++)
                {
                    vec3 p3 = vecFromUblas<vec3>(m.vertices[f[j]]);
                    *b++ = p3.cast<float>();
                }
            }
            offset += m.faces.size() * sizeof(vec3f) * 3;
        }

        if (!m.tc.empty())
        { // internal, separated
            spec.attributes[1].enable = true;
            spec.attributes[1].components = 2;
            spec.attributes[1].offset = offset;
            vec2f *b = (vec2f*)(spec.vertices.data() + offset);
            for (vtslibs::vts::Point3u32 f : m.facesTc)
                for (uint32 j = 0; j < 3; j++)
                    *b++ = vecFromUblas<vec2f>(m.tc[f[j]]);
            offset += m.faces.size() * sizeof(vec2f) * 3;
        }

        if (!m.etc.empty())
        { // external, interleaved
            spec.attributes[2].enable = true;
            spec.attributes[2].components = 2;
            spec.attributes[2].offset = offset;
            vec2f *b = (vec2f*)(spec.vertices.data() + offset);
            for (vtslibs::vts::Point3u32 f : m.faces)
                for (uint32 j = 0; j < 3; j++)
                    *b++ = vecFromUblas<vec2f>(m.etc[f[j]]);
            offset += m.faces.size() * sizeof(vec2f) * 3;
        }

        MeshPart part;
        part.renderable = gm;
        part.normToPhys = findNormToPhys(meshes[mi].extents) 
                * scaleMatrix(map->options.renderTilesScale);
        part.internalUv = spec.attributes[1].enable;
        part.externalUv = spec.attributes[2].enable;
        part.textureLayer = m.textureLayer ? *m.textureLayer : 0;
        part.surfaceReference = m.surfaceReference;
        submeshes.push_back(part);

        if (map->options.debugExtractRawResources)
        {
            static const std::string prefix = "extracted/";
            std::string b, c;
            std::string path = prefix
                    + convertNameToFolderAndFile(this->name, b, c)
                    + "_" + tmp + ".obj";
            if (!boost::filesystem::exists(path))
            {
                boost::filesystem::create_directories(prefix + b);
                vtslibs::vts::SubMesh msh(m);

                // denormalize vertex positions
                for (auto &v : msh.vertices)
                {
                    v = vecToUblas<math::Point3>(vec4to3(part.normToPhys
                                    * vec3to4(vecFromUblas<vec3>(v), 1)));
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

        map->callbacks.loadMesh(gm->info, spec);
        gm->state = Resource::State::ready;
    }

    info.ramMemoryCost += sizeof(*this) + meshes.size() * sizeof(MeshPart);
    for (auto &it : submeshes)
    {
        info.gpuMemoryCost += it.renderable->info.gpuMemoryCost;
        info.ramMemoryCost += it.renderable->info.ramMemoryCost;
    }
}

} // namespace vts
