#ifndef FETCHER_H_wjehfduj
#define FETCHER_H_wjehfduj

#include <string>
#include <memory>
#include <functional>
#include <map>

#include "foundation.hpp"
#include "buffer.hpp"

namespace vts
{

class MapImpl;

class VTS_API FetchTask
{
public:
    FetchTask(const std::string &name);
    virtual ~FetchTask();

    // query
    std::string queryUrl;
    std::map<std::string, std::string> queryHeaders;

    // reply
    std::string replyRedirectUrl;
    std::string contentType;
    Buffer contentData;
    uint32 replyCode;

    // internals
    uint32 redirectionsCount;
    const std::string name;
    
    void saveToCache(MapImpl *map);
    bool loadFromCache(MapImpl *map);
    void loadFromInternalMemory();
};

class VTS_API FetcherOptions
{
public:
    FetcherOptions();
    
    uint32 threads;
    sint32 timeout;
    
    // curl options
    uint32 maxHostConnections;
    uint32 maxTotalConections;
    uint32 maxCacheConections;
    sint32 pipelining;
};

class VTS_API Fetcher
{
public:
    static Fetcher *create(const FetcherOptions &options);
    
    typedef std::function<void(std::shared_ptr<FetchTask>)> Func;
    
    virtual ~Fetcher();
    
    virtual void initialize(Func func) = 0;
    virtual void finalize() = 0;
    virtual void fetch(std::shared_ptr<FetchTask>) = 0;
};

} // namespace vts

#endif
