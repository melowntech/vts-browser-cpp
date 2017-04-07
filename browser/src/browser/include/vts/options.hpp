#ifndef OPTIONS_H_kwegfdzvgsdfj
#define OPTIONS_H_kwegfdzvgsdfj

#include "foundation.hpp"

namespace vts
{

class VTS_API MapOptions
{
public:
    MapOptions();
    ~MapOptions();
    
    double autoRotateSpeed;
    double maxTexelToPixelScale;
    uint32 maxResourcesMemory;
    uint32 maxConcurrentDownloads;
    uint32 maxNodeUpdatesPerFrame;
    uint32 navigationSamplesPerViewExtent;
    bool renderWireBoxes;
    bool renderSurrogates;
    bool renderObjectPosition;
};

} // namespace vts

#endif
