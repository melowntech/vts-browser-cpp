#ifndef FETCHTASK_H_erbgfhufjgf
#define FETCHTASK_H_erbgfhufjgf

#include <string>
#include <atomic>
#include <vts-libs/registry/referenceframe.hpp>
#include <boost/optional.hpp>

#include "include/vts-browser/fetcher.hpp"

namespace vts
{

class MapImpl;

class FetchTaskImpl : public FetchTask
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
    
    FetchTaskImpl(MapImpl *map, const std::string &name,
                 FetchTask::ResourceType resourceType);
    virtual ~FetchTaskImpl();

    bool performAvailTest() const;
    bool allowDiskCache() const;
    void saveToCache();
    bool loadFromCache();
    void loadFromInternalMemory();
    virtual void fetchDone() override;

    const std::string name;
    MapImpl *const map;
    boost::optional<vtslibs::registry::BoundLayer::Availability> availTest;
    std::atomic<State> state;
    uint32 redirectionsCount;
};

} // namespace vts

#endif
