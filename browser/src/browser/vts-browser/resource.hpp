#ifndef RESOURCE_H_seghioqnh
#define RESOURCE_H_seghioqnh

#include <string>
#include <atomic>

#include <vts-libs/registry/referenceframe.hpp>
#include <boost/optional.hpp>

#include <vts/resources.hpp>
#include <vts/fetcher.hpp>

namespace vts
{

bool availableInCache(const std::string &name);

class ResourceImpl : public FetchTask
{
public:
    enum class State
    {
        initializing,
        downloading,
        downloaded,
        ready,
        errorDownload,
        errorLoad,
        finalizing,
    };
    
    ResourceImpl(Resource *resource);
    
    void saveToCache();
    void loadFromCache();
    void loadFromInternalMemory();
    
    Resource *const resource;
    vtslibs::registry::BoundLayer::Availability *availTest;
    uint32 lastAccessTick;
    std::atomic<State> state;
};

} // namespace vts

#endif
