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

class VTS_API FetchTask
{
public:
    enum class ResourceType
    {
        General,
        MapConfig,
        AuthConfig,
        BoundLayerConfig,
        BoundMetaTile,
        BoundMaskTile,
        MetaTile,
        Mesh,
        MeshPart,
        Texture,
        NavTile,
        Search,
    };
    const ResourceType resourceType;
    
    // query
    std::string queryUrl;
    std::map<std::string, std::string> queryHeaders;

    // reply
    Buffer contentData;
    std::string contentType;
    std::string replyRedirectUrl;
    uint32 replyCode;

    FetchTask(const std::string &url, ResourceType resourceType);
    virtual ~FetchTask();
    virtual void fetchDone() = 0;
};

class VTS_API FetcherOptions
{
public:
    FetcherOptions();

    uint32 threads;
    sint32 timeout;
    bool extraFileLog;

    // curl options
    uint32 maxHostConnections;
    uint32 maxTotalConections;
    uint32 maxCacheConections;
    sint32 pipelining; // 0, 1, 2 or 3
};

class VTS_API Fetcher
{
public:
    static std::shared_ptr<Fetcher> create(const FetcherOptions &options);

    virtual ~Fetcher();
    virtual void initialize() = 0;
    virtual void finalize() = 0;
    virtual void fetch(const std::shared_ptr<FetchTask> &) = 0;
};

} // namespace vts

#endif
