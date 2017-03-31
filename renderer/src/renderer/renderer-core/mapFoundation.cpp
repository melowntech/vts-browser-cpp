#include <renderer/map.h>

#include "map.h"

namespace melown
{

MapFoundation::MapFoundation()
{
    impl = std::shared_ptr<MapImpl>(new MapImpl(this));
}

MapFoundation::~MapFoundation()
{}

void MapFoundation::dataInitialize(Fetcher *fetcher)
{
    dbglog::thread_id("data");
    impl->dataInitialize(fetcher);
}

bool MapFoundation::dataTick()
{
    return impl->dataTick();
}

void MapFoundation::dataFinalize()
{
    impl->dataFinalize();
}

void MapFoundation::renderInitialize()
{
    dbglog::thread_id("render");
    impl->dataRenderInitialize();
    impl->renderInitialize();
}

void MapFoundation::renderTick(uint32 width, uint32 height)
{
    impl->statistics.resetFrame();
    impl->dataRenderTick();
    impl->renderTick(width, height);
}

void MapFoundation::renderFinalize()
{
    impl->dataRenderFinalize();
    impl->renderFinalize();
}

void MapFoundation::setMapConfig(const std::string &mapConfigPath)
{
    impl->setMapConfig(mapConfigPath);
}

void MapFoundation::pan(const double value[3])
{
    impl->pan(vec3(value[0], value[1], value[2]));
}

void MapFoundation::rotate(const double value[3])
{
    impl->rotate(vec3(value[0], value[1], value[2])); 
}

MapStatistics &MapFoundation::statistics()
{
    return impl->statistics;
}

MapOptions &MapFoundation::options()
{
    return impl->options;
}

DrawBatch &MapFoundation::drawBatch()
{
    return impl->draws;
}

MapImpl::MapImpl(MapFoundation *const mapFoundation) :
    mapFoundation(mapFoundation), initialized(false)
{}

} // namespace melown
