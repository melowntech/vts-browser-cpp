#include "map.h"
#include "resource.h"
#include "mapConfig.h"

namespace melown
{

MapConfig::MapConfig(const std::string &name) : Resource(name)
{}

void MapConfig::load(MapImpl *)
{
    vtslibs::vts::loadMapConfig(*this, impl->download->contentData, name);
}

ExternalBoundLayer::ExternalBoundLayer(const std::string &name)
    : Resource(name)
{}

void ExternalBoundLayer::load(MapImpl *)
{
    *(vtslibs::registry::BoundLayer*)this
        = vtslibs::registry::loadBoundLayer(impl->download->contentData, name);
}

} // namespace melown
