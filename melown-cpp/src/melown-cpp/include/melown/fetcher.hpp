#ifndef FETCHER_H_wjehfduj
#define FETCHER_H_wjehfduj

#include <string>
#include <functional>

#include "foundation.hpp"
#include "buffer.hpp"

namespace melown
{

struct MELOWN_API FetchTask
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

class MELOWN_API Fetcher
{
public:
    static Fetcher *create();
    
    typedef std::function<void(FetchTask *)> Func;
    
    virtual ~Fetcher();
    
    virtual void initialize(Func func) = 0;
    virtual void finalize() = 0;
    
    virtual void fetch(FetchTask *) = 0;
};

} // namespace melown

#endif
