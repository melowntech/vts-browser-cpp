#include <unordered_map>
#include <unordered_set>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <boost/filesystem.hpp>

#include <renderer/fetcher.h>

#include "cache.h"

#include "dbglog/dbglog.hpp"

namespace melown
{
    class Buffer
    {
    public:
        Buffer() : data(nullptr), size(0) {}
        Buffer(const Buffer &other) : data(nullptr), size(other.size) { data = malloc(size); memcpy(data, other.data, size); }
        Buffer(Buffer &&other) : data(other.data), size(other.size) { other.data = nullptr; other.size = 0; }
        ~Buffer() { free(data); data = nullptr; }
        Buffer &operator = (const Buffer &other) { if (&other == this) return *this; free(data); size = other.size; data = malloc(size); memcpy(data, other.data, size); }
        Buffer &operator = (Buffer &&other) { if (&other == this) return *this; free(data); size = other.size; data = other.data; other.data = nullptr; other.size = 0; }

        void *data;
        uint32 size;
    };

    class CacheImpl : public Cache
    {
    public:
        enum class Status
        {
            initialized,
            downloading,
            ready,
            done,
            error,
        };

        CacheImpl(Fetcher *fetcher) : fetcher(fetcher)
        {
            Fetcher::Func func = std::bind(&CacheImpl::fetchedFile, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
            fetcher->setCallback(func);
        }

        ~CacheImpl()
        {}

        const std::string convertNameToPath(const std::string &path)
        {
            std::string res;
            res.reserve(path.size());
            for (char it : path)
            {
                if ((it >= 'a' && it <= 'z')
                 || (it >= 'A' && it <= 'Z')
                 || (it >= '0' && it <= '9')
                 || (it == '-' || it == '.'))
                    res += it;
                else
                    res += '_';
            }
            return res;
        }

        const std::string convertNameToCache(const std::string &path)
        {
            return std::string("cache/") + convertNameToPath(path);
        }

        Result result(Status status)
        {
            switch (status)
            {
            case Status::initialized: return Result::downloading;
            case Status::downloading: return Result::downloading;
            case Status::ready: return Result::ready;
            case Status::done: return Result::ready;
            case Status::error: return Result::error;
            default:
                throw "invalid cache data status";
            }
        }

        Buffer readLocalFileBuffer(const std::string &path)
        {
            FILE *f = fopen(path.c_str(), "rb");
            if (!f)
                throw "failed to read file";
            Buffer b;
            fseek(f, 0, SEEK_END);
            b.size = ftell(f);
            fseek(f, 0, SEEK_SET);
            b.data = malloc(b.size);
            if (!b.data)
                throw "out of memory";
            if (fread(b.data, b.size, 1, f) != 1)
                throw "failed to read file";
            fclose(f);
            return b;
        }

        void readLocalFile(const std::string &name, const std::string &path)
        {
            try
            {
                data[name] = readLocalFileBuffer(path);
                states[name] = Status::ready;
            }
            catch (...)
            {
                states[name] = Status::error;
            }
        }

        void writeLocalFile(const std::string &path, const Buffer &buffer)
        {
            boost::filesystem::create_directories(boost::filesystem::path(path).parent_path());
            FILE *f = fopen(path.c_str(), "wb");
            if (!f)
                throw "failed to write file";
            if (fwrite(buffer.data, buffer.size, 1, f) != 1)
            {
                fclose(f);
                throw "failed to write file";
            }
            if (fclose(f) != 0)
                throw "failed to write file";
        }

        void fetchedFile(const std::string &name, const char *buffer, uint32 size) override
        {
            if (!buffer)
            {
                states[name] = Status::error;
                return;
            }
            Buffer b;
            b.size = size;
            b.data = malloc(size);
            memcpy(b.data, buffer, size);
            writeLocalFile(convertNameToCache(name), b);
            data[name] = b;
            states[name] = Status::ready;
        }

        Result read(const std::string &name, void *&buffer, uint32 &size) override
        {
            if (states[name] == Status::initialized)
            {
                states[name] = Status::downloading;
                if (name.find("://") == std::string::npos)
                    readLocalFile(name, name);
                else
                {
                    std::string cachePath = convertNameToCache(name);
                    if (boost::filesystem::exists(cachePath))
                        readLocalFile(name, cachePath);
                    else
                        fetcher->fetch(name);
                }
            }
            switch (states[name])
            {
            case Status::done:
            case Status::ready:
            {
                Buffer &buf = data[name];
                buffer = buf.data;
                size = buf.size;
                states[name] = Status::done;
            } break;
            }
            return result(states[name]);
        }

        std::unordered_map<std::string, Status> states;
        std::unordered_map<std::string, Buffer> data;
        Fetcher *fetcher;
    };

    Cache *Cache::create(class MapImpl *, Fetcher *fetcher)
    {
        return new CacheImpl(fetcher);
    }

    Cache::~Cache()
    {}
}
