#ifndef STATISTICS_H_wqieufhbvgjh
#define STATISTICS_H_wqieufhbvgjh

#include "foundation.hpp"

namespace melown
{

class MELOWN_API MapStatistics
{
public:
    MapStatistics();
    ~MapStatistics();
    void resetAll();
    void resetFrame();

    static const uint32 MaxLods = 22;

    // frame statistics

    uint32 meshesRenderedTotal;
    uint32 meshesRenderedPerLod[MaxLods];
    uint32 metaNodesTraversedTotal;
    uint32 metaNodesTraversedPerLod[MaxLods];

    // global statistics

    uint32 resourcesIgnored;
    uint32 resourcesDownloaded;
    uint32 resourcesDiskLoaded;
    uint32 resourcesProcessLoaded;
    uint32 resourcesReleased;
    uint32 resourcesFailed;
    uint32 frameIndex;
    
    // current statistics
    
    uint32 currentDownloads;
    uint32 currentResources;
    uint32 currentGpuMemUse;
    uint32 currentRamMemUse;
    uint32 lastHeightRequestLod;
    uint32 currentNodeUpdates;
    double currentNearPlane;
    double currentFarPlane;
};

} // namespace melown

#endif
