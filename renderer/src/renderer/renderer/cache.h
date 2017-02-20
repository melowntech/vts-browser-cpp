#ifndef CACHE_H_seghioqnh
#define CACHE_H_seghioqnh

#include <string>

#include <renderer/foundation.h>

namespace melown
{
    class Cache
    {
    public:
        static Cache *create(class MapImpl *map, class Fetcher *fetcher);

        virtual bool read(const std::string &name, void *&buffer, uint32 &size) = 0;
        virtual void fetchedFile(const std::string &name, const char *buffer, uint32 size) = 0;
    };
}

#endif
