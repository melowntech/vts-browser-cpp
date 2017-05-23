#ifndef RESOURCES_H_ergiuhdusgju
#define RESOURCES_H_ergiuhdusgju

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vts-libs/vts/mapconfig.hpp>
#include <vts-libs/vts/urltemplate.hpp>
#include <vts-libs/vts/metatile.hpp>

#include "include/vts-browser/resources.hpp"
#include "include/vts-browser/math.hpp"
#include "csConvertor.hpp"
#include "fetchTask.hpp"

namespace vts
{

class Resource
{
public:
    Resource();
    virtual ~Resource();
    virtual void load() = 0;
    operator bool () const;
    
    ResourceInfo info;
    std::shared_ptr<FetchTaskImpl> impl;
};

class GpuMesh : public Resource
{
public:
    void load() override;
};

class GpuTexture : public Resource
{
public:
    void load() override;
};

class AuthConfig : public Resource
{
public:
    AuthConfig();
    void load() override;
    void checkTime();
    void authorize(FetchTaskImpl *task);

private:
    std::string token;
    std::unordered_set<std::string> hostnames;
    uint64 timeValid;
    uint64 timeParsed;
};

class MapConfig : public Resource, public vtslibs::vts::MapConfig
{
public:
    class BoundInfo : public vtslibs::registry::BoundLayer
    {
    public:
        BoundInfo(const BoundLayer &layer);
    
        vtslibs::vts::UrlTemplate urlExtTex;
        vtslibs::vts::UrlTemplate urlMeta;
        vtslibs::vts::UrlTemplate urlMask;
    };
    
    class SurfaceInfo : public vtslibs::vts::SurfaceCommonConfig
    {
    public:
        SurfaceInfo(const SurfaceCommonConfig &surface,
                    const std::string &parentPath);
    
        vtslibs::vts::UrlTemplate urlMeta;
        vtslibs::vts::UrlTemplate urlMesh;
        vtslibs::vts::UrlTemplate urlIntTex;
        vtslibs::vts::UrlTemplate urlNav;
        vtslibs::vts::TilesetIdList name;
    };
    
    class SurfaceStackItem
    {
    public:
        SurfaceStackItem();
        
        std::shared_ptr<SurfaceInfo> surface;
        vec3f color;
        bool alien;
    };
    
    class BrowserOptions
    {
    public:
        BrowserOptions();
        
        double autorotate;
    };
    
    MapConfig();
    void load() override;
    void clear();
    static const std::string convertPath(const std::string &path,
                                         const std::string &parent);
    
    vtslibs::vts::SurfaceCommonConfig *findGlue(
            const vtslibs::vts::Glue::Id &id);
    vtslibs::vts::SurfaceCommonConfig *findSurface(const std::string &id);
    BoundInfo *getBoundInfo(const std::string &id);
    void printSurfaceStack();
    void generateSurfaceStack();
    
    std::unordered_map<std::string, std::shared_ptr<SurfaceInfo>> surfaceInfos;
    std::unordered_map<std::string, std::shared_ptr<BoundInfo>> boundInfos;
    std::vector<SurfaceStackItem> surfaceStack;
    std::shared_ptr<CsConvertor> convertor;
    BrowserOptions browserOptions;
};

class ExternalBoundLayer : public Resource,
        public vtslibs::registry::BoundLayer
{
public:
    void load() override;
};

class BoundMetaTile : public Resource
{
public:
    void load() override;

    uint8 flags[vtslibs::registry::BoundLayer::rasterMetatileWidth
                * vtslibs::registry::BoundLayer::rasterMetatileHeight];
};

class BoundMaskTile : public Resource
{
public:
    void load() override;

    std::shared_ptr<GpuTexture> texture;
};

class MetaTile : public Resource, public vtslibs::vts::MetaTile
{
public:
    MetaTile();
    void load() override;
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
    void load() override;

    std::vector<MeshPart> submeshes;
};

class NavTile : public Resource
{
public:
    void load() override;
    
    std::vector<unsigned char> data;
    
    static vec2 sds2px(const vec2 &point, const math::Extents2 &extents);
};

} // namespace vts

#endif
