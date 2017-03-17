#include <vts-libs/vts/meshio.hpp>

#include <renderer/gpuContext.h>
#include <renderer/gpuResources.h>

#include "map.h"
#include "resource.h"
#include "mapResources.h"
#include "resourceManager.h"
#include "math.h"
#include "image.h"

namespace melown
{

MetaTile::MetaTile(const std::string &name) : Resource(name),
    vtslibs::vts::MetaTile(vtslibs::vts::TileId(), 0)
{}

void MetaTile::load(MapImpl *base)
{
    *(vtslibs::vts::MetaTile*)this
            = vtslibs::vts::loadMetaTile(impl->download->contentData, 5, name);
    ramMemoryCost = this->size() * sizeof(vtslibs::vts::MetaNode);
}

NavTile::NavTile(const std::string &name) : Resource(name)
{}

void NavTile::load(MapImpl *base)
{
    RawNavTile::deserialize({0, 0}, impl->download->contentData, name);
}

MeshPart::MeshPart() : textureLayer(0), surfaceReference(0),
    internalUv(false), externalUv(false)
{}

MeshAggregate::MeshAggregate(const std::string &name) : Resource(name)
{}

namespace {
const mat4 findNormToPhys(const math::Extents3 &extents)
{
    vec3 u = vecFromUblas<vec3>(extents.ur);
    vec3 l = vecFromUblas<vec3>(extents.ll);
    vec3 d = (u - l) * 0.5;
    vec3 c = (u + l) * 0.5;
    mat4 sc = scaleMatrix(d(0), d(1), d(2));
    mat4 tr = translationMatrix(c);
    return tr * sc;
}
}

void MeshAggregate::load(MapImpl *base)
{
    vtslibs::vts::NormalizedSubMesh::list meshes = vtslibs::vts::
            loadMeshProperNormalized(impl->download->contentData, name);

    submeshes.clear();
    submeshes.reserve(meshes.size());

    for (uint32 mi = 0, me = meshes.size(); mi != me; mi++)
    {
        vtslibs::vts::SubMesh &m = meshes[mi].submesh;

        char tmp[10];
        sprintf(tmp, "%d", mi);
        std::shared_ptr<GpuMeshRenderable> gm
                = std::dynamic_pointer_cast<GpuMeshRenderable>
                (base->resources->dataContext->createMeshRenderable
                 (name + "#" + tmp));

        uint32 vertexSize = sizeof(vec3f);
        if (m.tc.size())
            vertexSize += sizeof(vec2f);
        if (m.etc.size())
            vertexSize += sizeof(vec2f);

        GpuMeshSpec spec;
        spec.verticesCount = m.faces.size() * 3;
        spec.vertices.allocate(spec.verticesCount * vertexSize);
        uint32 offset = 0;

        { // vertices
            spec.attributes[0].enable = true;
            spec.attributes[0].components = 3;
            vec3f *b = (vec3f*)spec.vertices.data();
            for (vtslibs::vts::Point3u32 f : m.faces)
            {
                for (uint32 j = 0; j < 3; j++)
                {
                    vec3 p3 = vecFromUblas<vec3>(m.vertices[f[j]]);
                    *b++ = p3.cast<float>();
                }
            }
            offset += m.faces.size() * sizeof(vec3f) * 3;
        }

        if (!m.tc.empty())
        { // internal, separated
            spec.attributes[1].enable = true;
            spec.attributes[1].components = 2;
            spec.attributes[1].offset = offset;
            vec2f *b = (vec2f*)(spec.vertices.data() + offset);
            for (vtslibs::vts::Point3u32 f : m.facesTc)
                for (uint32 j = 0; j < 3; j++)
                    *b++ = vecFromUblas<vec2f>(m.tc[f[j]]);
            offset += m.faces.size() * sizeof(vec2f) * 3;
        }

        if (!m.etc.empty())
        { // external, interleaved
            spec.attributes[2].enable = true;
            spec.attributes[2].components = 2;
            spec.attributes[2].offset = offset;
            vec2f *b = (vec2f*)(spec.vertices.data() + offset);
            for (vtslibs::vts::Point3u32 f : m.faces)
                for (uint32 j = 0; j < 3; j++)
                    *b++ = vecFromUblas<vec2f>(m.etc[f[j]]);
            offset += m.faces.size() * sizeof(vec2f) * 3;
        }

        gm->loadMeshRenderable(spec);

        MeshPart part;
        part.renderable = gm;
        part.normToPhys = findNormToPhys(meshes[mi].extents);
        part.internalUv = spec.attributes[1].enable;
        part.externalUv = spec.attributes[2].enable;
        part.textureLayer = m.textureLayer ? *m.textureLayer : 0;
        part.surfaceReference = m.surfaceReference;
        submeshes.push_back(part);
    }

    gpuMemoryCost = 0;
    ramMemoryCost = meshes.size() * sizeof(MeshPart);
    for (auto &&it : submeshes)
    {
        gpuMemoryCost += it.renderable->gpuMemoryCost;
        ramMemoryCost += it.renderable->ramMemoryCost;
    }
}

BoundMetaTile::BoundMetaTile(const std::string &name) : Resource(name)
{}

void BoundMetaTile::load(MapImpl *)
{
    Buffer buffer = std::move(impl->download->contentData);
    GpuTextureSpec spec;
    decodeImage(name, buffer, spec.buffer,
                spec.width, spec.height, spec.components);
    if (spec.buffer.size() != sizeof(flags))
        throw std::runtime_error("bound meta tile has invalid resolution");
    memcpy(flags, spec.buffer.data(), spec.buffer.size());
    ramMemoryCost = spec.buffer.size();
}

BoundMaskTile::BoundMaskTile(const std::string &name) : Resource(name)
{}

void BoundMaskTile::load(MapImpl *base)
{
    if (!texture)
        texture = std::dynamic_pointer_cast<GpuTexture>(
                    base->resources->dataContext->createTexture(name + "#tex"));
    
    Buffer buffer = std::move(impl->download->contentData);
    GpuTextureSpec spec;
    decodeImage(name, buffer, spec.buffer,
                spec.width, spec.height, spec.components);
    texture->loadTexture(spec);
    gpuMemoryCost = texture->gpuMemoryCost;
    ramMemoryCost = texture->ramMemoryCost;
}

} // namespace melown
