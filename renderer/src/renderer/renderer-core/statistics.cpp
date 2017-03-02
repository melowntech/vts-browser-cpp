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
    LOG(info3) << "metaNodesTraverzedTotal:" << metaNodesTraverzedTotal;
    for (uint32 i = 0; i < MaxLods; i++)
        LOG(info3) << "metaNodesTraverzedPerLod[" << i << "]:"
                   << metaNodesTraverzedPerLod[i];
}

void MapStatistics::resetAll()
{
    resetFrame();
    resourcesDownloaded = 0;
    resourcesDiskLoaded = 0;
}

void MapStatistics::resetFrame()
{
    meshesRenderedTotal = 0;
    metaNodesTraverzedTotal = 0;
    for (uint32 i = 0; i < MaxLods; i++)
    {
        meshesRenderedPerLod[i] = 0;
        metaNodesTraverzedPerLod[i] = 0;
    }
}

} // namespace melown
