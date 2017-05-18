#ifndef RESOURCE_H_seghioqnh
#define RESOURCE_H_seghioqnh

#include <string>
#include <atomic>
#include <vts-libs/registry/referenceframe.hpp>
#include <boost/optional.hpp>

#include "include/vts-browser/resources.hpp"
#include "include/vts-browser/fetcher.hpp"

namespace vts
{

class ResourceImpl : public FetchTask
{
public:
    enum class State
    {
        initializing,
        downloading,
        downloaded,
        ready,
        error,
        finalizing,
    };
    
    ResourceImpl(const std::string &name, bool allowDiskCache);
    virtual ~ResourceImpl();

    bool performAvailTest() const;
    
    boost::optional<vtslibs::registry::BoundLayer::Availability> availTest;
    std::atomic<State> state;
    double priority;
    uint32 lastAccessTick;
    uint32 ramMemoryCost;
    uint32 gpuMemoryCost;
    bool allowDiskCache;
};

} // namespace vts

#endif
