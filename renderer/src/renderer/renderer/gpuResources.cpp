#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>

#include <renderer/gpuResources.h>

#include "map.h"
#include "cache.h"
#include "resourceManager.h"

namespace melown
{
    GpuShader::GpuShader(const std::string &name) : Resource(name)
    {}

    void GpuShader::load(MapImpl *base)
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

    GpuTexture::GpuTexture(const std::string &name) : Resource(name)
    {}

    void GpuTexture::load(MapImpl *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        if (base->cache->read(name, buffer, size))
            loadTexture(buffer, size);
    }

    GpuMeshSpec::GpuMeshSpec() : indexBufferData(nullptr), vertexBufferData(nullptr), verticesCount(0), vertexBufferSize(0), indicesCount(0), faceMode(FaceMode::Triangles)
    {}

    GpuMeshSpec::VertexAttribute::VertexAttribute() : offset(0), stride(0), components(0), type(Type::Float), enable(false), normalized(false)
    {}

    GpuMeshRenderable::GpuMeshRenderable(const std::string &name) : Resource(name)
    {}

    void GpuMeshRenderable::load(MapImpl *base)
    {}
}
