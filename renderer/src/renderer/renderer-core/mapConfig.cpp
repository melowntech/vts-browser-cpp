#include "map.h"
#include "resource.h"
#include "mapConfig.h"

namespace melown
{

MapConfig::MapConfig(const std::string &name) : Resource(name)
{}

void MapConfig::load(MapImpl *)
{
    Buffer buffer = std::move(impl->download->contentData);
    std::istringstream is(std::string((char*)buffer.data, buffer.size));
    vtslibs::vts::loadMapConfig(*this, is, name);
}

ExternalBoundLayer::ExternalBoundLayer(const std::string &name)
    : Resource(name)
{}

void ExternalBoundLayer::load(MapImpl *)
{
    Buffer buffer = std::move(impl->download->contentData);
    std::istringstream is(std::string((char*)buffer.data, buffer.size));
    *(vtslibs::registry::BoundLayer*)this
            = vtslibs::registry::loadBoundLayer(is, name);
}

} // namespace melown
