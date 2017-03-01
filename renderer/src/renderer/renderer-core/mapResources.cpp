#include <renderer/gpuContext.h>
#include <renderer/gpuResources.h>

#include "map.h"
#include "cache.h"
#include "mapResources.h"
#include "resourceManager.h"
#include "math.h"

#include <vts-libs/vts/meshio.hpp>

namespace melown
{

MetaTile::MetaTile(const std::string &name) : Resource(name),
    vtslibs::vts::MetaTile(vtslibs::vts::TileId(), 0)
{}

void MetaTile::load(MapImpl *base)
{
    Buffer buffer;
    switch (base->cache->read(name, buffer))
    {
    case Cache::Result::ready:
    {
        std::istringstream is(std::string((char*)buffer.data, buffer.size));
        *(vtslibs::vts::MetaTile*)this
                = vtslibs::vts::loadMetaTile(is, 5, name);
        state = State::ready;
        return;
    }
    case Cache::Result::error:
        state = State::errorDownload;
        return;
    }
}

MeshPart::MeshPart() : textureLayer(0), internalUv(false), externalUv(false)
{}

MeshAggregate::MeshAggregate(const std::string &name) : Resource(name)
{}

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

void MeshAggregate::load(MapImpl *base)
{
    Buffer buffer;
    switch (base->cache->read(name, buffer))
    {
    case Cache::Result::ready:
    {
        std::istringstream is(std::string((char*)buffer.data, buffer.size));
        vtslibs::vts::NormalizedSubMesh::list meshes
                = vtslibs::vts::loadMeshProperNormalized(is, name);

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
            spec.vertexBufferSize = spec.verticesCount * vertexSize;

            Buffer buffer(spec.vertexBufferSize);
            spec.vertexBufferData = buffer.data;
            uint32 offset = 0;

            { // vertices
                spec.attributes[0].enable = true;
                spec.attributes[0].components = 3;
                vec3f *b = (vec3f*)buffer.data;
                for (vtslibs::vts::Point3u32 f : m.faces)
                    for (uint32 j = 0; j < 3; j++)
                        *b++ = vecFromUblas<vec3f>(m.vertices[f[j]]);
                offset += m.faces.size() * sizeof(vec3f) * 3;
            }

            if (!m.tc.empty())
            { // internal, separated
                spec.attributes[1].enable = true;
                spec.attributes[1].components = 2;
                spec.attributes[1].offset = offset;
                vec2f *b = (vec2f*)(((char*)buffer.data) + offset);
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
                vec2f *b = (vec2f*)(((char*)buffer.data) + offset);
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
            submeshes.push_back(part);
        }

        gpuMemoryCost = 0;
        bool ready = true;
        for (auto &&it : submeshes)
        {
            ready = ready && it.renderable->state == Resource::State::ready;
            gpuMemoryCost += it.renderable->gpuMemoryCost;
        }
        state = ready ? State::ready : State::errorLoad;
        return;
    }
    case Cache::Result::error:
        state = State::errorDownload;
        return;
    }
}

ExternalBoundLayer::ExternalBoundLayer(const std::string &name)
    : Resource(name)
{}

void ExternalBoundLayer::load(MapImpl *base)
{
    Buffer buffer;
    switch (base->cache->read(name, buffer))
    {
    case Cache::Result::ready:
    {
        std::istringstream is(std::string((char*)buffer.data, buffer.size));
        bl = vtslibs::registry::loadBoundLayer(is, name);
        state = State::ready;
        return;
    }
    case Cache::Result::error:
        state = State::errorDownload;
        return;
    }
}

} // namespace melown
