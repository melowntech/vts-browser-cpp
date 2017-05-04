#ifndef FETCHER_H_wjehfduj
#define FETCHER_H_wjehfduj

#include <string>
#include <memory>
#include <functional>

#include "foundation.hpp"
#include "buffer.hpp"

namespace vts
{

struct VTS_API FetchTask
{
    FetchTask(const std::string &url);
    virtual ~FetchTask();
    
    // query
    std::string url;
    bool allowRedirects;
    
    // reply
    std::string redirectUrl;
    std::string contentType;
    Buffer contentData;
    uint32 code;
    uint32 redirectionsCount;
};

class VTS_API Fetcher
{
public:
    static Fetcher *create(uint32 threads = 2, sint32 timeout = 10000);
    
    typedef std::function<void(std::shared_ptr<FetchTask>)> Func;
    
    virtual ~Fetcher();
    
    virtual void initialize(Func func) = 0;
    virtual void finalize() = 0;
    
    virtual void fetch(std::shared_ptr<FetchTask>) = 0;
};

} // namespace vts

#endif
