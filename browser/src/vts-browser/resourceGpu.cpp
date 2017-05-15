#include <vts-libs/registry/referenceframe.hpp>
#include <vts/resources.hpp>
#include <vts/buffer.hpp>
#include <vts/math.hpp>

#include "resource.hpp"
#include "image.hpp"
#include "obj.hpp"

namespace vts
{

GpuTextureSpec::GpuTextureSpec() : width(0), height(0), components(0),
    verticalFlip(true)
{}

GpuTextureSpec::GpuTextureSpec(const Buffer &buffer)
{
    decodeImage(buffer, this->buffer, width, height, components);
}

GpuTexture::GpuTexture(const std::string &name) : Resource(name, true)
{}

void GpuTexture::load(class MapImpl *)
{
    LOG(info2) << "Loading gpu texture '" << impl->name << "'";
    GpuTextureSpec spec(impl->contentData);
    spec.verticalFlip = true;
    loadTexture(spec);
}

GpuMeshSpec::GpuMeshSpec() : verticesCount(0), indicesCount(0),
    faceMode(FaceMode::Triangles)
{}

GpuMeshSpec::GpuMeshSpec(const Buffer &buffer) :
    verticesCount(0), indicesCount(0),
    faceMode(FaceMode::Triangles)
{
    uint32 dummy;
    uint32 fm;
    decodeObj(buffer, fm, vertices, indices, verticesCount, dummy);
    switch (fm)
    {
    case 1:
        faceMode = FaceMode::Points;
        break;
    case 2:
        faceMode = FaceMode::Lines;
        break;
    case 3:
        faceMode = FaceMode::Triangles;
        break;
    default:
        LOGTHROW(fatal, std::invalid_argument) << "Invalid face mode";
    }
}

GpuMeshSpec::VertexAttribute::VertexAttribute() : offset(0), stride(0),
    components(0), type(Type::Float), enable(false), normalized(false)
{}

GpuMesh::GpuMesh(const std::string &name) : Resource(name, true)
{}

void GpuMesh::load(class MapImpl *)
{
    LOG(info2) << "Loading gpu mesh '" << impl->name << "'";
    GpuMeshSpec spec(impl->contentData);
    spec.attributes[0].enable = true;
    spec.attributes[0].stride = sizeof(vec3f) + sizeof(vec2f);
    spec.attributes[0].components = 3;
    spec.attributes[1].enable = true;
    spec.attributes[1].stride = spec.attributes[0].stride;
    spec.attributes[1].components = 2;
    spec.attributes[1].offset = sizeof(vec3f);
    spec.attributes[2] = spec.attributes[1];
    loadMesh(spec);
}

} // namespace vts
