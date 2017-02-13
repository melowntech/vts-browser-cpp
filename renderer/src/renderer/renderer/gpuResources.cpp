#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>

#include "map.h"
#include "cache.h"
#include "gpuManager.h"
#include "gpuResources.h"

#include "../../vts-libs/vts/mesh.hpp"

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
        if (base->gpuManager->cache->read(name + ".vert.glsl", buffer, size))
        {
            std::string vert((char*)buffer, size);
            buffer = nullptr;
            size = 0;
            if (base->gpuManager->cache->read(name + ".frag.glsl", buffer, size))
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
        if (base->gpuManager->cache->read(name, buffer, size))
            loadTexture(buffer, size);
    }

    template<class Tout, class Tin> void convertPoints(std::vector<Tout> &out, const std::vector<Tin> &in)
    {
        out.resize(in.size());
        for (uint32 i = 0, e = in.size(); i != e; i++)
            out[i] = in[i];
    }

    void GpuMeshAggregate::loadToGpu(const std::string &name, Map *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        if (base->gpuManager->cache->read(name, buffer, size))
        {
            std::istringstream is(std::string((char*)buffer, size));
            vadstena::vts::Mesh mesh = vadstena::vts::loadMesh(is, name);
            for (uint32 mi = 0, me = mesh.size(); mi != me; mi++)
            {
                char tmp[10];
                sprintf(tmp, "%d", mi);
                GpuSubMesh *gm = base->gpuManager->getSubMesh(name + "#" + tmp);
                vadstena::vts::SubMesh &m = mesh[mi];
                math::Points3f vertices;
                convertPoints(vertices, m.vertices);
                // todo uv
                GpuMeshSpec spec;
                spec.vertexBuffer = vertices.data();
                spec.vertexCount = vertices.size();
                spec.vertexSize = sizeof(math::Point3f);
                // todo faces
                // todo spec.attributes
                gm->loadSubMesh(spec);
            }
            loadMeshAggregate(); // update memoryCost
        }
    }
}
