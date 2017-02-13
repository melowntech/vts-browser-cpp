#include <unordered_map>
#include <unordered_set>
#include <cstring>

#include "cache.h"
#include "fetcher.h"

namespace melown
{
    class Buffer
    {
    public:
        Buffer() : data(nullptr), size(0) {}
        Buffer(const Buffer &other) : data(nullptr), size(other.size) { data = malloc(size); memcpy(data, other.data, size); }
        ~Buffer() { free(data); data = nullptr; }
        Buffer &operator = (const Buffer &other) { if (&other == this) return *this; free(data); size = other.size; data = malloc(size); memcpy(data, other.data, size); }
        // todo move constructor and move assignment

        void *data;
        uint32 size;
    };

    class CacheImpl : public Cache
    {
    public:
        CacheImpl(Fetcher *fetcher) : fetcher(fetcher)
        {
            Fetcher::Func func = std::bind(&CacheImpl::fetchedFile, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
            fetcher->setCallback(func);
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
            downloading.erase(name);
            buffer = it->second.data;
            size = it->second.size;
            return true;
        }

        void fetchedFile(FetchType type, const std::string &name, const char *buffer, uint32 size) override
        {
            Buffer b;
            b.data = malloc(size);
            memcpy(b.data, buffer, size);
            b.size = size;
            data[name] = b;
        }

        std::unordered_set<std::string> downloading;
        std::unordered_map<std::string, Buffer> data;
        Fetcher *fetcher;
    };

    Cache *Cache::create(Fetcher *fetcher)
    {
        return new CacheImpl(fetcher);
    }

}
