#ifndef RESOURCE_H_seghioqnh
#define RESOURCE_H_seghioqnh

#include <string>
#include <atomic>

#include <vts-libs/registry/referenceframe.hpp>
#include <boost/optional.hpp>

#include <renderer/foundation.h>
#include <renderer/resource.h>
#include <renderer/fetcher.h>

namespace melown
{

class ResourceImpl
{
public:
    enum class State
    {
        initializing,
        preparing,
        ready,
        errorDownload,
        errorLoad,
        finalizing,
    };
    
    class DownloadTask : public FetchTask
    {
    public:
        DownloadTask(ResourceImpl *resource);

        ResourceImpl *const resource;
        
        void saveToCache();
        bool loadFromCache();
        void readLocalFile();
    };
    
    ResourceImpl(Resource *resource);
    
    boost::optional<DownloadTask> download;
    Resource *const resource;
    vtslibs::registry::BoundLayer::Availability *availTest;
    uint32 lastAccessTick;
    std::atomic<State> state;
};

} // namespace melown

#endif
