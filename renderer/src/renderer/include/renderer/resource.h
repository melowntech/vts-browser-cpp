#ifndef RESOURCE_H_hjsghjefjkn
#define RESOURCE_H_hjsghjefjkn

#include <string>
#include <memory>

#include "foundation.h"

namespace melown
{

class ResourceImpl;

class MELOWN_API Resource
{
public:

    Resource(const std::string &name);
    virtual ~Resource();

    virtual void load(class MapImpl *base) = 0;

    operator bool () const;
    
    const std::string name;
    uint32 ramMemoryCost;
    uint32 gpuMemoryCost;
    
    std::shared_ptr<ResourceImpl> impl;
};

} // namespace melown

#endif
