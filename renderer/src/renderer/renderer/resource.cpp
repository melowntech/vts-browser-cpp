#include <renderer/resource.h>

namespace melown
{
    Resource::Resource(const std::string &name) : name(name), ramMemoryCost(0), gpuMemoryCost(0), state(State::initializing)
    {}

    Resource::~Resource()
    {}
}
