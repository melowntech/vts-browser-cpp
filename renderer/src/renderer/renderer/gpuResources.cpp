#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>

#include "map.h"
#include "cache.h"
#include "gpuManager.h"
#include "gpuResources.h"

#include "../../vts-libs/vts/meshio.hpp"

namespace melown
{
    GpuResource::GpuResource() : memoryCost(0), ready(false)
    {}

    GpuResource::~GpuResource()
    {}

    void GpuResource::loadToGpu(const std::string &name, Map *base)
    {}

    GpuMeshSpec::GpuMeshSpec() : indexBuffer(nullptr), vertexBuffer(nullptr), vertexCount(0), vertexSize(0), indexCount(0), faceMode(FaceMode::Triangles)
    {}

    GpuMeshSpec::VertexAttribute::VertexAttribute() : offset(0), stride(0), components(0), type(Type::Float), enable(false), normalized(false)
    {}

    void GpuShader::loadToGpu(const std::string &name, Map *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        if (base->cache->read(name + ".vert.glsl", buffer, size))
        {
            std::string vert((char*)buffer, size);
            buffer = nullptr;
            size = 0;
            if (base->cache->read(name + ".frag.glsl", buffer, size))
            {
                std::string frag((char*)buffer, size);
                loadShaders(vert, frag);
            }
        }
    }

    void GpuTexture::loadToGpu(const std::string &name, Map *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        if (base->cache->read(name, buffer, size))
            loadTexture(buffer, size);
    }

    /*
    template<class Tout, class Tin> void convertPoints(std::vector<Tout> &out, const std::vector<Tin> &in)
    {
        out.resize(in.size());
        for (uint32 i = 0, e = in.size(); i != e; i++)
            out[i] = in[i];
    }
    */

    void GpuMeshAggregate::loadToGpu(const std::string &name, Map *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        if (base->cache->read(name, buffer, size))
        {
            struct VertexAttributes
            {
                math::Point3f vert;
                math::Point2f uv;
            };
            std::vector<VertexAttributes> attributes;

            std::istringstream is(std::string((char*)buffer, size));
            vadstena::vts::Mesh mesh;
            vadstena::vts::detail::loadMeshProper(is, name, mesh);
            for (uint32 mi = 0, me = mesh.size(); mi != me; mi++)
            {
                char tmp[10];
                sprintf(tmp, "%d", mi);
                GpuSubMesh *gm = base->gpuManager->getSubMesh(name + "#" + tmp);
                vadstena::vts::SubMesh &m = mesh[mi];

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
                            at.vert = m.vertices[k];
                            at.uv = m.etc[k];
                            attributes.push_back(at);
                        }
                    }
                } break;
                case vadstena::vts::SubMesh::internal: // texture coordinates are separately
                {
                    for (uint32 i = 0, e = m.faces.size(); i != e; i++)
                    {
                        for (uint32 j = 0; j < 3; j++)
                        {
                            VertexAttributes at;
                            at.vert = m.vertices[m.faces[i][j]];
                            at.uv = m.tc[m.facesTc[i][j]];
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
                spec.attributes[1].offset = sizeof(math::Point3f);

                gm->loadSubMesh(spec);
            }
            loadMeshAggregate(); // update memoryCost
        }
    }
}
