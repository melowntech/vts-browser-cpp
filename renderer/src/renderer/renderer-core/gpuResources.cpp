#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>

#include <renderer/gpuResources.h>

#include "map.h"
#include "cache.h"
#include "resourceManager.h"
#include "buffer.h"
#include "image.h"

namespace melown
{
    GpuShader::GpuShader(const std::string &name) : Resource(name)
    {}

    void GpuShader::load(MapImpl *base)
    {
        Buffer bv, bf;
        Cache::Result rv = base->cache->read(name + ".vert.glsl", bv);
        Cache::Result rf = base->cache->read(name + ".frag.glsl", bf);
        if (rv == Cache::Result::error || rf == Cache::Result::error)
        {
            state = State::errorDownload;
            return;
        }
        if (rv == Cache::Result::ready && rf == Cache::Result::ready)
        {
            std::string vert((char*)bv.data, bv.size);
            std::string frag((char*)bf.data, bf.size);
            loadShaders(vert, frag);
        }
    }

    GpuTextureSpec::GpuTextureSpec() : width(0), height(0), components(0), bufferSize(0), buffer(nullptr)
    {}

    GpuTexture::GpuTexture(const std::string &name) : Resource(name)
    {}

    void GpuTexture::load(MapImpl *base)
    {
        Buffer encoded;
        switch (base->cache->read(name, encoded))
        {
        case Cache::Result::ready:
        {
            GpuTextureSpec spec;
            Buffer decoded;
            decodeImage(name, encoded, decoded, spec.width, spec.height, spec.components);
            spec.buffer = decoded.data;
            spec.bufferSize = decoded.size;
            loadTexture(spec);
        } return;
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
