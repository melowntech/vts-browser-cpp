#ifndef MAPCONFIG_H_qwedfzcbk
#define MAPCONFIG_H_qwedfzcbk

#include <string>
#include <unordered_map>

#include <vts-libs/registry/referenceframe.hpp>
#include <vts-libs/vts/mapconfig.hpp>
#include <vts-libs/vts/urltemplate.hpp>
#include <melown/resources.hpp>

#include "csConvertor.hpp"
#include "math.hpp"

namespace melown
{

const std::string convertPath(const std::string &path,
                              const std::string &parent);

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

class MapConfig : public Resource, public vtslibs::vts::MapConfig
{
public:
    MapConfig(const std::string &name);

    void load(class MapImpl *base) override;
    
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
};

class ExternalBoundLayer : public Resource,
        public vtslibs::registry::BoundLayer
{
public:
    ExternalBoundLayer(const std::string &name);

    void load(class MapImpl *base) override;
};

} // namespace melown

#endif
