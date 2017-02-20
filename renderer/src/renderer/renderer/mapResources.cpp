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
        if (base->cache->read(name, buffer, size))
        {
            std::istringstream is(std::string((char*)buffer, size));
            *(vadstena::vts::MetaTile*)this = vadstena::vts::loadMetaTile(is, 5, name);
            state = State::ready;
        }
    }

    MeshAggregate::MeshAggregate(const std::__cxx11::string &name) : Resource(name)
    {}

    void MeshAggregate::load(MapImpl *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        if (base->cache->read(name, buffer, size))
        {
            struct VertexAttributes
            {
                vec3f vert;
                vec2f uv;
            };
            std::vector<VertexAttributes> attributes;

            std::istringstream is(std::string((char*)buffer, size));
            vadstena::vts::Mesh mesh;
            vadstena::vts::detail::loadMeshProper(is, name, mesh);

            submeshes.clear();
            submeshes.reserve(mesh.size());

            for (uint32 mi = 0, me = mesh.size(); mi != me; mi++)
            {
                vadstena::vts::SubMesh &m = mesh[mi];

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

                /*
                { // find real bounding box
                    vec3 a(FLT_MAX, FLT_MAX, FLT_MAX);
                    vec3 b(-a);
                    for (auto &&v : m.vertices)
                    {
                        a = min(a, vecFromUblas<vec3>(v));
                        b = max(b, vecFromUblas<vec3>(v));
                    }
                    LOG(info3) << "bbox: " << a(0) << " " << a(1) << " " << a(2) << " --- " << b(0) << " " << b(1) << " " << b(2);
                    vec3 c = (a + b) * 0.5;
                    LOG(info3) << "center: " << c(0) << " " << c(1) << " " << c(2);
                }
                */

                gm->loadMeshRenderable(spec);

                submeshes.push_back(gm);
            }

            gpuMemoryCost = 0;
            bool ready = true;
            for (auto &&it : submeshes)
            {
                ready = ready && it->state == Resource::State::ready;
                gpuMemoryCost += it->gpuMemoryCost;
            }
            state = ready ? State::ready : State::errorLoad;
        }
    }
}
