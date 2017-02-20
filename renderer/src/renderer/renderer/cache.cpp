#include <unordered_map>
#include <unordered_set>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <renderer/fetcher.h>

#include "cache.h"

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
        CacheImpl(Fetcher *fetcher) : fetcher(fetcher)
        {
            Fetcher::Func func = std::bind(&CacheImpl::fetchedFile, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
            fetcher->setCallback(func);
        }

        ~CacheImpl()
        {}

        void readLocalFile(const std::string &name)
        {
            FILE *f = fopen(name.c_str(), "rb");
            if (!f)
            {
                states[name] = Result::error;
                return;
            }
            Buffer b;
            fseek(f, 0, SEEK_END);
            b.size = ftell(f);
            fseek(f, 0, SEEK_SET);
            b.data = malloc(b.size);
            fread(b.data, b.size, 1, f);
            fclose(f);
            data[name] = b;
            states[name] = Result::ready;
        }

        Result read(const std::string &name, void *&buffer, uint32 &size) override
        {
            Result state = states[name];
            switch (state)
            {
            case Result::initialized:
            {
                states[name] = Result::downloading;
                if (name.find("://") == std::string::npos)
                    readLocalFile(name);
                else
                    fetcher->fetch(name);
            } break;
            case Result::ready:
            {
                Buffer &buf = data[name];
                buffer = buf.data;
                size = buf.size;
            } break;
            }
            return state;
        }

        void fetchedFile(const std::string &name, const char *buffer, uint32 size) override
        {
            if (!buffer)
            {
                states[name] = Result::error;
                return;
            }
            Buffer b;
            b.size = size;
            b.data = malloc(size);
            memcpy(b.data, buffer, size);
            data[name] = b;
            states[name] = Result::ready;
        }

        std::unordered_map<std::string, Result> states;
        std::unordered_map<std::string, Buffer> data;
        Fetcher *fetcher;
    };

    Cache *Cache::create(class MapImpl *, Fetcher *fetcher)
    {
        return new CacheImpl(fetcher);
    }

}
