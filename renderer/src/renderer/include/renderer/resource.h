#ifndef RESOURCE_H_hjsghjefjkn
#define RESOURCE_H_hjsghjefjkn

#include <string>

#include "foundation.h"

namespace melown
{

class MELOWN_API Resource
{
public:
    enum class State
    {
        initializing,
        ready,
        errorDownload,
        errorLoad,
        finalizing,
    };

    Resource(const std::string &name);
    virtual ~Resource();

    virtual void load(class MapImpl *base) = 0;

    const std::string name;
    uint32 ramMemoryCost;
    uint32 gpuMemoryCost;
    uint32 lastAccessTick;
    State state;
};

} // namespace melown

#endif
