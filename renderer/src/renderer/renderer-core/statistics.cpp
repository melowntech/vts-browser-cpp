#include <dbglog/dbglog.hpp>

#include <renderer/statistics.h>

namespace melown
{

MapStatistics::MapStatistics()
    : currentDownloads(0)
{
    resetAll();
}

MapStatistics::~MapStatistics()
{
    LOG(info3) << "currentDownloads:" << currentDownloads;
    LOG(info3) << "resourcesIgnored:" << resourcesIgnored;
    LOG(info3) << "resourcesDownloaded:" << resourcesDownloaded;
    LOG(info3) << "resourcesDiskLoaded:" << resourcesDiskLoaded;
    LOG(info3) << "resourcesGpuLoaded:" << resourcesGpuLoaded;
    LOG(info3) << "resourcesReleased:" << resourcesReleased;
    LOG(info3) << "resourcesFailed:" << resourcesFailed;
    LOG(info3) << "meshesRenderedTotal:" << meshesRenderedTotal;
    for (uint32 i = 0; i < MaxLods; i++)
        if (meshesRenderedPerLod[i])
            LOG(info3) << "meshesRenderedPerLod[" << i << "]:"
                   << meshesRenderedPerLod[i];
    LOG(info3) << "metaNodesTraversedTotal:" << metaNodesTraversedTotal;
    for (uint32 i = 0; i < MaxLods; i++)
        if (metaNodesTraversedPerLod[i])
            LOG(info3) << "metaNodesTraversedPerLod[" << i << "]:"
                   << metaNodesTraversedPerLod[i];
    LOG(info3) << "frameIndex:" << frameIndex;
}

void MapStatistics::resetAll()
{
    resetFrame();
    resourcesIgnored = 0;
    resourcesDownloaded = 0;
    resourcesDiskLoaded = 0;
    resourcesGpuLoaded = 0;
    resourcesReleased = 0;
    resourcesFailed = 0;
    frameIndex = 0;
}

void MapStatistics::resetFrame()
{
    meshesRenderedTotal = 0;
    metaNodesTraversedTotal = 0;
    for (uint32 i = 0; i < MaxLods; i++)
    {
        meshesRenderedPerLod[i] = 0;
        metaNodesTraversedPerLod[i] = 0;
    }
}

} // namespace melown
