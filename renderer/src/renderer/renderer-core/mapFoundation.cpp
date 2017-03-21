#include <renderer/map.h>

#include "map.h"

namespace melown
{

MapFoundation::MapFoundation(const std::string &mapConfigPath)
{
    impl = std::shared_ptr<MapImpl>(new MapImpl(mapConfigPath));
}

MapFoundation::~MapFoundation()
{}

void MapFoundation::dataInitialize(GpuContext *context, Fetcher *fetcher)
{
    dbglog::thread_id("data");
    impl->dataInitialize(context, fetcher);
}

bool MapFoundation::dataTick()
{
    return impl->dataTick();
}

void MapFoundation::dataFinalize()
{
    impl->dataFinalize();
}

void MapFoundation::renderInitialize(GpuContext *context)
{
    dbglog::thread_id("render");
    impl->dataRenderInitialize();
    impl->renderInitialize(context);
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

MapImpl::MapImpl(const std::string &mapConfigPath)
    : mapConfigPath(mapConfigPath)
{}

} // namespace melown
