#include <vts/statistics.hpp>

namespace vts
{

MapStatistics::MapStatistics()
{
    resetAll();
}

MapStatistics::~MapStatistics()
{}

void MapStatistics::resetAll()
{
    resetFrame();
    resourcesIgnored = 0;
    resourcesDownloaded = 0;
    resourcesDiskLoaded = 0;
    resourcesProcessLoaded = 0;
    resourcesReleased = 0;
    resourcesFailed = 0;
    frameIndex = 0;
    currentResourceDownloads = 0;
    currentGpuMemUse = 0;
    currentRamMemUse = 0;
    currentResources = 0;
    lastHeightRequestLod = 0;
    currentResourcePreparing = 0;
}

void MapStatistics::resetFrame()
{
    currentNodeUpdates = 0;
    meshesRenderedTotal = 0;
    metaNodesTraversedTotal = 0;
    for (uint32 i = 0; i < MaxLods; i++)
    {
        meshesRenderedPerLod[i] = 0;
        metaNodesTraversedPerLod[i] = 0;
    }
}

} // namespace vts
