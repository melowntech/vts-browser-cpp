#include <dbglog/dbglog.hpp>

#include <renderer/statistics.h>

namespace melown
{

MapStatistics::MapStatistics()
{
    resetAll();
}

MapStatistics::~MapStatistics()
{
    LOG(info3) << "resourcesDownloaded:" << resourcesDownloaded;
    LOG(info3) << "resourcesDiskLoaded:" << resourcesDiskLoaded;
    LOG(info3) << "meshesRenderedTotal:" << meshesRenderedTotal;
    for (uint32 i = 0; i < MaxLods; i++)
        LOG(info3) << "meshesRenderedPerLod[" << i << "]:"
                   << meshesRenderedPerLod[i];
    LOG(info3) << "metaNodesTraversedTotal:" << metaNodesTraversedTotal;
    for (uint32 i = 0; i < MaxLods; i++)
        LOG(info3) << "metaNodesTraversedPerLod[" << i << "]:"
                   << metaNodesTraversedPerLod[i];
    LOG(info3) << "frameIndex:" << frameIndex;
}

void MapStatistics::resetAll()
{
    resetFrame();
    resourcesDownloaded = 0;
    resourcesDiskLoaded = 0;
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
