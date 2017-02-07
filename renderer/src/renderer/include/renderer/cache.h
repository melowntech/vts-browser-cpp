#ifndef CACHE_H_wjehfduj
#define CACHE_H_wjehfduj

#include <string>
#include <memory>

#include "foundation.h"

namespace melown
{
    class MELOWN_API Cache
    {
    public:
        virtual ~Cache();
        virtual void request(const std::string &name) = 0;
        virtual void purge(const std::string &name) = 0;
        virtual bool available(const std::string &name) const = 0;
        virtual bool read(const std::string &name, std::shared_ptr<char> &buffer, uint32 &size) = 0;
    };

    MELOWN_API const std::shared_ptr<Cache> newDefaultCache(const std::string &hddPath = "cache", uint64 hddLimit = 4000000000, uint64 memoryLimit = 1000000000);
}

#endif
