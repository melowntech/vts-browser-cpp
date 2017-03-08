#include <unordered_map>
#include <unordered_set>
#include <cstdio>

#include <boost/filesystem.hpp>
#include <dbglog/dbglog.hpp>

#include <renderer/fetcher.h>
#include <renderer/statistics.h>

#include "cache.h"
#include "buffer.h"
#include "map.h"

namespace melown
{

class CacheImpl : public Cache
{
public:
    enum class Status
    {
        initialized,
        downloading,
        ready,
        error,
    };

    CacheImpl(MapImpl *map, Fetcher *fetcher) : fetcher(fetcher), map(map),
        downloadingTasks(0)
    {
        if (!fetcher)
            return;
        Fetcher::Func func = std::bind(&CacheImpl::fetchedFile, this,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3);
        fetcher->setCallback(func);
        
        try
        {
            Buffer buf = readLocalFileBuffer("cache/invalidUrl.txt");
            std::istringstream is(std::string((char*)buf.data, buf.size));
            while (is.good())
            {
                std::string line;
                std::getline(is, line);
                states[line] = Status::error;
            }
        }
        catch (...)
        {
            // do nothing
        }
    }

    ~CacheImpl()
    {
        try
        {
            boost::filesystem::create_directories("cache");
            FILE *f = fopen("cache/invalidUrl.txt", "wb");
            if (!f)
                throw std::runtime_error("failed to write file");
            for (auto it : states)
            {
                if (it.second == Status::error)
                    fprintf(f, "%s\n", it.first.c_str());
            }
            if (fclose(f) != 0)
                throw std::runtime_error("failed to write file");
        }
        catch (...)
        {
            // do nothing
        }
    }

    const std::string convertNameToPath(std::string path, bool preserveSlashes)
    {
        path = boost::filesystem::path(path).normalize().string();
        std::string res;
        res.reserve(path.size());
        for (char it : path)
        {
            if ((it >= 'a' && it <= 'z')
             || (it >= 'A' && it <= 'Z')
             || (it >= '0' && it <= '9')
             || (it == '-' || it == '.'))
                res += it;
            else if (preserveSlashes && (it == '/' || it == '\\'))
                res += '/';
            else
                res += '_';
        }
        return res;
    }

    const std::string convertNameToCache(const std::string &path)
    {
        uint32 p = path.find("://");
        std::string a = p == std::string::npos ? path : path.substr(p + 3);
        std::string b = boost::filesystem::path(a).parent_path().string();
        std::string c = a.substr(b.length() + 1);
        return std::string("cache/")
                + convertNameToPath(b, false) + "/"
                + convertNameToPath(c, false);
    }

    Result result(Status status)
    {
        switch (status)
        {
        case Status::initialized: return Result::downloading;
        case Status::downloading: return Result::downloading;
        case Status::ready: return Result::ready;
        case Status::error: return Result::error;
        default:
            throw std::invalid_argument("invalid cache data status");
        }
    }

    Buffer readLocalFileBuffer(const std::string &path)
    {
        FILE *f = fopen(path.c_str(), "rb");
        if (!f)
            throw std::runtime_error("failed to read file");
        try
        {
            Buffer b;
            fseek(f, 0, SEEK_END);
            b.size = ftell(f);
            fseek(f, 0, SEEK_SET);
            b.data = malloc(b.size);
            if (!b.data)
                throw std::runtime_error("out of memory");
            if (fread(b.data, b.size, 1, f) != 1)
                throw std::runtime_error("failed to read file");
            fclose(f);
            return b;
        }
        catch (...)
        {
            fclose(f);
            throw;
        }
    }

    void readLocalFile(const std::string &name, const std::string &path)
    {
        try
        {
            data[name] = readLocalFileBuffer(path);
            states[name] = Status::ready;
        }
        catch (std::runtime_error &)
        {
            states[name] = Status::error;
        }
    }

    void writeLocalFile(const std::string &path, const Buffer &buffer)
    {
        boost::filesystem::create_directories(
                    boost::filesystem::path(path).parent_path());
        FILE *f = fopen(path.c_str(), "wb");
        if (!f)
            throw std::runtime_error("failed to write file");
        if (fwrite(buffer.data, buffer.size, 1, f) != 1)
        {
            fclose(f);
            throw std::runtime_error("failed to write file");
        }
        if (fclose(f) != 0)
            throw std::runtime_error("failed to write file");
    }

    void fetchedFile(const std::string &name,
                     const char *buffer, uint32 size) override
    {
        downloadingTasks--;
        if (!buffer)
        {
            states[name] = Status::error;
            return;
        }
        Buffer b;
        b.size = size;
        b.data = malloc(size);
        if (!b.data)
            throw std::runtime_error("out of memory");
        memcpy(b.data, buffer, size);
        writeLocalFile(convertNameToCache(name), b);
        data[name] = std::move(b);
        states[name] = Status::ready;
    }

    Result read(const std::string &name, Buffer &buffer,
                bool allowDiskCache) override
    {
        if (states[name] == Status::initialized)
        {
            readyToRemove.erase(name);
            states[name] = Status::downloading;
            if (name.find("://") == std::string::npos)
                readLocalFile(name, name);
            else
            {
                std::string cachePath = convertNameToCache(name);
                if (allowDiskCache && boost::filesystem::exists(cachePath))
                {
                    readLocalFile(name, cachePath);
                    map->statistics->resourcesDiskLoaded++;
                }
                else if (downloadingTasks < 10)
                {
                    fetcher->fetch(name);
                    downloadingTasks++;
                    map->statistics->resourcesDownloaded++;
                }
                else
                    states[name] = Status::initialized;
            }
        }
        if (states[name] == Status::ready)
        {
            buffer = std::move(data[name]);
            states[name] = Status::initialized;
            readyToRemove.insert(name);
            return result(Status::ready);
        }
        return result(states[name]);
    }
    
    void tick() override
    {
        for (auto n : readyToRemove)
        {
            states.erase(n);
            data.erase(n);
        }
    }

    std::unordered_map<std::string, Status> states;
    std::unordered_map<std::string, Buffer> data;
    std::unordered_set<std::string> readyToRemove;
    Fetcher *fetcher;
    MapImpl *map;
    uint32 downloadingTasks;
};

Cache *Cache::create(MapImpl *map, Fetcher *fetcher)
{
    return new CacheImpl(map, fetcher);
}

Cache::~Cache()
{}

} // namespace melown
