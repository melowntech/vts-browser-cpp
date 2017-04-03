#ifndef OPTIONS_H_kwegfdzvgsdfj
#define OPTIONS_H_kwegfdzvgsdfj

#include "foundation.h"

namespace melown
{

class MELOWN_API MapOptions
{
public:
    MapOptions();
    ~MapOptions();
    
    double maxTexelToPixelScale;
    uint32 maxResourcesMemory;
    uint32 maxConcurrentDownloads;
    uint32 navigationSamplesPerViewExtent;
    bool renderWireBoxes;
    bool renderSurrogates;
    bool renderObjectPosition;
};

} // namespace melown

#endif
