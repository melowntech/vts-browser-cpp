#include "map.h"
#include "cache.h"
#include "mapResources.h"
#include "gpuResources.h"
#include "resourceManager.h"
#include "gpuContext.h"

#include "../../vts-libs/vts/meshio.hpp"

namespace melown
{
    void MapConfig::load(const std::string &name, class Map *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        if (base->cache->read(name, buffer, size))
        {
            std::istringstream is(std::string((char*)buffer, size));
            vadstena::vts::loadMapConfig(*this, is, name);
            basePath = name.substr(0, name.find_last_of('/') + 1);
            ready = true;
        }
    }

    MetaTile::MetaTile() : vadstena::vts::MetaTile(vadstena::vts::TileId(), 0)
    {}

    void MetaTile::load(const std::string &name, class Map *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        if (base->cache->read(name, buffer, size))
        {
            std::istringstream is(std::string((char*)buffer, size));
            *(vadstena::vts::MetaTile*)this = vadstena::vts::loadMetaTile(is, 5, name);
            ready = true;
        }
    }

    void MeshAggregate::load(const std::string &name, Map *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        if (base->cache->read(name, buffer, size))
        {
            struct VertexAttributes
            {
                vec3 vert;
                vec2 uv;
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
                std::shared_ptr<GpuMeshRenderable> gm = std::dynamic_pointer_cast<GpuMeshRenderable>(base->resources->dataContext->createMeshRenderable());

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
                            at.vert = vecFromUblas<vec3>(m.vertices[k]);
                            at.uv = vecFromUblas<vec2>(m.etc[k]);
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
                            at.vert = vecFromUblas<vec3>(m.vertices[m.faces[i][j]]);
                            at.uv = vecFromUblas<vec2>(m.tc[m.facesTc[i][j]]);
                            attributes.push_back(at);
                        }
                    }
                } break;
                default:
                    throw "invalid texture mode";
                }

                GpuMeshSpec spec;
                spec.vertexBuffer = attributes.data();
                spec.vertexCount = attributes.size();
                spec.vertexSize = sizeof(VertexAttributes);

                // vertex coordinates
                spec.attributes[0].enable = true;
                spec.attributes[0].stride = sizeof(VertexAttributes);
                spec.attributes[0].components = 3;

                // uv coordinates
                spec.attributes[1].enable = true;
                spec.attributes[1].stride = sizeof(VertexAttributes);
                spec.attributes[1].components = 2;
                spec.attributes[1].offset = sizeof(vec3);

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

                gm->loadMeshRenderable(spec);

                submeshes.push_back(gm);
            }

            gpuMemoryCost = 0;
            for (auto &&it : submeshes)
            {
                ready = ready && it->ready;
                gpuMemoryCost += it->gpuMemoryCost;
            }
            ready = true;
        }
    }
}
