#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>

#include <vts-libs/registry/referenceframe.hpp>

#include <renderer/gpuResources.h>
#include <renderer/buffer.h>

#include "map.h"
#include "resource.h"
#include "resourceManager.h"
#include "image.h"
#include "obj.h"
#include "math.h"

namespace melown
{

GpuShader::GpuShader(const std::string &name) : Resource(name)
{}

void GpuShader::load(MapImpl *)
{
    Buffer buffer = std::move(impl->download->contentData);
    std::istringstream is(std::string((char*)buffer.data, buffer.size));
    std::map<std::string, std::string> shaders;
    std::string current;
    while (is.good())
    {
        std::string line;
        std::getline(is, line);
        if (line.empty())
            continue;
        if (line[0] == '$')
            current = line.substr(1);
        else
            shaders[current] += line + "\n";
    }
    loadShaders(shaders["vertex"], shaders["fragment"]);
}

GpuTextureSpec::GpuTextureSpec() : width(0), height(0), components(0)
{}

GpuTexture::GpuTexture(const std::string &name) : Resource(name)
{}

void GpuTexture::load(MapImpl *)
{
    Buffer buffer = std::move(impl->download->contentData);
    GpuTextureSpec spec;
    decodeImage(name, buffer, spec.buffer,
                spec.width, spec.height, spec.components);
    loadTexture(spec);
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
    Buffer buffer = std::move(impl->download->contentData);
    uint32 vc = 0, ic = 0;
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
}

} // namespace melown
