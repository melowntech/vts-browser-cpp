#include <dbglog/dbglog.hpp>

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
    currentDownloads = 0;
    currentGpuMemUse = 0;
    currentRamMemUse = 0;
    currentResources = 0;
}

void MapStatistics::resetFrame()
{
    lastHeightRequestLod = 0;
    currentNearPlane = 0;
    currentFarPlane = 0;
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
