#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>

#include "map.h"
#include "cache.h"
#include "resourceManager.h"
#include "gpuResources.h"

namespace melown
{
    GpuMeshSpec::GpuMeshSpec() : indexBuffer(nullptr), vertexBuffer(nullptr), vertexCount(0), vertexSize(0), indexCount(0), faceMode(FaceMode::Triangles)
    {}

    GpuMeshSpec::VertexAttribute::VertexAttribute() : offset(0), stride(0), components(0), type(Type::Float), enable(false), normalized(false)
    {}

    void GpuShader::load(const std::string &name, Map *base)
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

    void GpuTexture::load(const std::string &name, Map *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        if (base->cache->read(name, buffer, size))
            loadTexture(buffer, size);
    }

    void GpuMeshRenderable::load(const std::string &name, Map *base)
    {}
}
