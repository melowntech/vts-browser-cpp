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
    
    uint32 maxResourcesMemory;
    uint32 maxConcurrentDownloads;
    bool renderWireBoxes;
};

} // namespace melown

#endif
