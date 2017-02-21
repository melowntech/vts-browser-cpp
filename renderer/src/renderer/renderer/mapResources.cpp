#include <renderer/gpuContext.h>
#include <renderer/gpuResources.h>

#include "map.h"
#include "cache.h"
#include "mapResources.h"
#include "resourceManager.h"
#include "math.h"

#include "../../vts-libs/vts/meshio.hpp"

namespace melown
{
    MetaTile::MetaTile(const std::string &name) : Resource(name), vadstena::vts::MetaTile(vadstena::vts::TileId(), 0)
    {}

    void MetaTile::load(MapImpl *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        switch (base->cache->read(name, buffer, size))
        {
        case Cache::Result::ready:
        {
            std::istringstream is(std::string((char*)buffer, size));
            *(vadstena::vts::MetaTile*)this = vadstena::vts::loadMetaTile(is, 5, name);
            state = State::ready;
            return;
        }
        case Cache::Result::error:
            state = State::errorDownload;
            return;
        }
    }

    MeshAggregate::MeshAggregate(const std::string &name) : Resource(name)
    {}

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

    void MeshAggregate::load(MapImpl *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        switch (base->cache->read(name, buffer, size))
        {
        case Cache::Result::ready:
        {
            struct VertexAttributes
            {
                vec3f vert;
                vec2f uv;
            };
            std::vector<VertexAttributes> attributes;

            std::istringstream is(std::string((char*)buffer, size));
            vadstena::vts::NormalizedSubMesh::list meshes = vadstena::vts::loadMeshProperNormalized(is, name);

            submeshes.clear();
            submeshes.reserve(meshes.size());

            for (uint32 mi = 0, me = meshes.size(); mi != me; mi++)
            {
                vadstena::vts::SubMesh &m = meshes[mi].submesh;

                char tmp[10];
                sprintf(tmp, "%d", mi);
                std::shared_ptr<GpuMeshRenderable> gm = std::dynamic_pointer_cast<GpuMeshRenderable>(base->resources->dataContext->createMeshRenderable(name + "#" + tmp));

                attributes.clear();
                attributes.reserve(m.faces.size());
                switch (m.textureMode)
                {
                case vadstena::vts::SubMesh::external: // texture coordinates are interleaved with vertices
                {
                    for (uint32 i = 0, e = m.faces.size(); i != e; i++)
                    {
                        for (uint32 j = 0; j < 3; j++)
                        {
                            VertexAttributes at;
                            uint32 k = m.faces[i][j];
                            at.vert = vecFromUblas<vec3f>(m.vertices[k]);
                            at.uv = vecFromUblas<vec2f>(m.etc[k]);
                            attributes.push_back(at);
                        }
                    }
                } break;
                case vadstena::vts::SubMesh::internal: // texture coordinates are separate
                {
                    for (uint32 i = 0, e = m.faces.size(); i != e; i++)
                    {
                        for (uint32 j = 0; j < 3; j++)
                        {
                            VertexAttributes at;
                            at.vert = vecFromUblas<vec3f>(m.vertices[m.faces[i][j]]);
                            at.uv = vecFromUblas<vec2f>(m.tc[m.facesTc[i][j]]);
                            attributes.push_back(at);
                        }
                    }
                } break;
                default:
                    throw "invalid texture mode";
                }

                GpuMeshSpec spec;
                spec.vertexBufferData = attributes.data();
                spec.vertexBufferSize = attributes.size() * sizeof(VertexAttributes);
                spec.verticesCount = attributes.size();

                // vertex coordinates
                spec.attributes[0].enable = true;
                spec.attributes[0].stride = sizeof(VertexAttributes);
                spec.attributes[0].components = 3;

                // uv coordinates
                spec.attributes[1].enable = true;
                spec.attributes[1].stride = sizeof(VertexAttributes);
                spec.attributes[1].components = 2;
                spec.attributes[1].offset = sizeof(vec3f);

                gm->loadMeshRenderable(spec);

                MeshPart part;
                part.renderable = gm;
                part.normToPhys = findNormToPhys(meshes[mi].extents);
                submeshes.push_back(part);

                //vec4 c = part.normToPhys * vec4(0, 0, 0, 1);
                //LOG(info3) << "center: " << c(0) << " " << c(1) << " " << c(2);
            }

            gpuMemoryCost = 0;
            bool ready = true;
            for (auto &&it : submeshes)
            {
                ready = ready && it.renderable->state == Resource::State::ready;
                gpuMemoryCost += it.renderable->gpuMemoryCost;
            }
            state = ready ? State::ready : State::errorLoad;
            return;
        }
        case Cache::Result::error:
            state = State::errorDownload;
            return;
        }
    }
}
