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
        void *bv = nullptr, *bf = nullptr;
        uint32 sv = 0, sf = 0;
        Cache::Result rv = base->cache->read(name + ".vert.glsl", bv, sv);
        Cache::Result rf = base->cache->read(name + ".frag.glsl", bf, sf);
        if (rv == Cache::Result::error || rf == Cache::Result::error)
        {
            state = State::errorDownload;
            return;
        }
        if (rv == Cache::Result::ready && rf == Cache::Result::ready)
        {
            std::string vert((char*)bv, sv);
            std::string frag((char*)bf, sf);
            loadShaders(vert, frag);
        }
    }

    GpuTexture::GpuTexture(const std::string &name) : Resource(name)
    {}

    void GpuTexture::load(MapImpl *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        switch (base->cache->read(name, buffer, size))
        {
        case Cache::Result::ready:
            loadTexture(buffer, size);
            return;
        case Cache::Result::error:
            state = State::errorDownload;
            return;
        }
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
