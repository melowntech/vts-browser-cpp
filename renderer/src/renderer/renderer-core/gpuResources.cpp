#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>

#include <renderer/gpuResources.h>
#include <renderer/buffer.h>

#include "map.h"
#include "cache.h"
#include "resourceManager.h"
#include "image.h"
#include "obj.h"
#include "math.h"

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

GpuTextureSpec::GpuTextureSpec() : width(0), height(0), components(0)
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
        decodeImage(name, encoded, spec.buffer,
                    spec.width, spec.height, spec.components);
        loadTexture(spec);
    } return;
    case Cache::Result::error:
        state = State::errorDownload;
        return;
    }
}

GpuMeshSpec::GpuMeshSpec() : verticesCount(0), indicesCount(0),
    faceMode(FaceMode::Triangles)
{}

GpuMeshSpec::VertexAttribute::VertexAttribute() : offset(0), stride(0),
    components(0), type(Type::Float), enable(false), normalized(false)
{}

GpuMeshRenderable::GpuMeshRenderable(const std::string &name) : Resource(name)
{}

void GpuMeshRenderable::load(MapImpl *base)
{
    Buffer buffer;
    switch (base->cache->read(name, buffer))
    {
    case Cache::Result::ready:
    {
        uint32 vc, ic;
        GpuMeshSpec spec;
        decodeObj(name, buffer, spec.vertices, spec.indices, vc, ic);
        spec.verticesCount = vc;
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vec3f) + sizeof(vec2f);
        spec.attributes[0].components = 3;
        spec.attributes[1].enable = true;
        spec.attributes[1].stride = spec.attributes[0].stride;
        spec.attributes[1].components = 2;
        spec.attributes[1].offset = sizeof(vec3f);
        spec.attributes[2] = spec.attributes[1];
        loadMeshRenderable(spec);
    } return;
    case Cache::Result::error:
        state = State::errorDownload;
        return;
    }
}

} // namespace melown
