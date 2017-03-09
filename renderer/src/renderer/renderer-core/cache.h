#ifndef CACHE_H_seghioqnh
#define CACHE_H_seghioqnh

#include <string>

#include <renderer/foundation.h>
#include <renderer/buffer.h>

namespace melown
{

class FetchTask;

class Cache
{
public:
    enum class Result
    {
        downloading,
        ready,
        error,
    };

    static Cache *create(class MapImpl *map, class Fetcher *fetcher);

    virtual ~Cache();

    virtual Result read(const std::string &name, Buffer &buffer,
                        bool allowDiskCache = true) = 0;

    virtual void fetchedFile(FetchTask *task) = 0;
    
    virtual void tick() = 0;
};

} // namespace melown

#endif
