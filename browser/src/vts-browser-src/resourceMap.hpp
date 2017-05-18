#ifndef MAPRESOURCES_H_hdsgfhjuwebgfj
#define MAPRESOURCES_H_hdsgfhjuwebgfj

#include <memory>
#include <string>
#include <vts-libs/vts/metatile.hpp>
#include <vts-libs/registry/referenceframe.hpp>

#include "include/vts-browser/resources.hpp"
#include "include/vts-browser/math.hpp"

namespace vts
{

class MetaTile : public Resource, public vtslibs::vts::MetaTile
{
public:
    MetaTile(const std::string &name);
    void load(class MapImpl *base) override;
};

class NavTile : public Resource
{
public:
    NavTile(const std::string &name);
    void load(class MapImpl *base) override;
    
    std::vector<unsigned char> data;
    
    static vec2 sds2px(const vec2 &point, const math::Extents2 &extents);
};

class MeshPart
{
public:
    MeshPart();
    std::shared_ptr<GpuMesh> renderable;
    mat4 normToPhys;
    uint32 textureLayer;
    uint32 surfaceReference;
    bool internalUv;
    bool externalUv;
};

class MeshAggregate : public Resource
{
public:
    MeshAggregate(const std::string &name);

    void load(class MapImpl *base) override;

    std::vector<MeshPart> submeshes;
};

class BoundMetaTile : public Resource
{
public:
    BoundMetaTile(const std::string &name);

    void load(class MapImpl *base) override;

    uint8 flags[vtslibs::registry::BoundLayer::rasterMetatileWidth
                * vtslibs::registry::BoundLayer::rasterMetatileHeight];
};

class BoundMaskTile : public Resource
{
public:
    BoundMaskTile(const std::string &name);

    void load(class MapImpl *base) override;

    std::shared_ptr<GpuTexture> texture;
};

} // namespace vts

#endif
