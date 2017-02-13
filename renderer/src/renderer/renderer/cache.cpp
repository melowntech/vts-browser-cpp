#include <unordered_map>
#include <unordered_set>
#include <cstring>

#include "cache.h"
#include "fetcher.h"
#include "map.h"
#include "mapConfig.h"

#include "dbglog/dbglog.hpp"

namespace melown
{
    class Buffer
    {
    public:
        ~Buffer() { free(data); data = nullptr; }
        Buffer() : data(nullptr), size(0) {}
        Buffer(const Buffer &other) : data(nullptr), size(other.size) { data = malloc(size); memcpy(data, other.data, size); }
        Buffer &operator = (const Buffer &other) { if (&other == this) return *this; free(data); size = other.size; data = malloc(size); memcpy(data, other.data, size); }
        Buffer &operator = (Buffer &&other) { if (&other == this) return *this; free(data); size = other.size; data = other.data; other.data = nullptr; other.size = 0; }

        void *data;
        uint32 size;
    };

    class CacheImpl : public Cache
    {
    public:
        CacheImpl(Map *map, Fetcher *fetcher) : map(map), fetcher(fetcher)
        {
            Fetcher::Func func = std::bind(&CacheImpl::fetchedFile, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
            fetcher->setCallback(func);
            fetcher->fetch(FetchType::MapConfig, map->mapConfigPath);
        }

        ~CacheImpl()
        {}

        bool read(const std::string &name, void *&buffer, uint32 &size) override
        {
            auto it = data.find(name);
            if (it == data.end())
            {
                if (downloading.find(name) == downloading.end())
                {
                    downloading.insert(name);
                    fetcher->fetch(FetchType::Binary, name);
                }
                return false;
            }
            buffer = it->second.data;
            size = it->second.size;
            return true;
        }

        void fetchedFile(FetchType type, const std::string &name, const char *buffer, uint32 size) override
        {
            LOG(info3) << "downloaded: " << name;

            downloading.erase(name);
            if (!buffer)
                throw "download failed";
            switch (type)
            {
            case FetchType::MapConfig:
            {
                MapConfig *mc = new MapConfig();
                parseMapConfig(mc, name, buffer, size);
                map->mapConfig = mc;
            } return;
            default: break;
            }
            Buffer b;
            b.data = malloc(size);
            memcpy(b.data, buffer, size);
            b.size = size;
            data[name] = b;
        }

        std::unordered_set<std::string> downloading;
        std::unordered_map<std::string, Buffer> data;
        Map *map;
        Fetcher *fetcher;
    };

    Cache *Cache::create(Map *map, Fetcher *fetcher)
    {
        return new CacheImpl(map, fetcher);
    }

}
