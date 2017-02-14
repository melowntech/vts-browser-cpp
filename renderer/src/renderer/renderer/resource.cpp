#include "resource.h"

namespace melown
{
    Resource::Resource() : ramMemoryCost(0), gpuMemoryCost(0), ready(false)
    {}

    Resource::~Resource()
    {}
}
