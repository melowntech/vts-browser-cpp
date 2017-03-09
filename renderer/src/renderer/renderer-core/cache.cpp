#include <unordered_map>
#include <unordered_set>
#include <cstdio>

#include <boost/atomic/atomic.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
#include <dbglog/dbglog.hpp>
#include <vts-libs/registry/referenceframe.hpp>

#include <renderer/statistics.h>
#include <renderer/buffer.h>
#include <renderer/fetcher.h>

#include "cache.h"
#include "map.h"

namespace melown
{

FetchTask::FetchTask(const std::string &name) : name(name), url(name), 
    allowRedirects(false), code(0), redirectionsCount(0)
{}

Fetcher::~Fetcher()
{}

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
    
    class CacheTask : public FetchTask
    {
    public:
        CacheTask(const std::string &name) : FetchTask(name),
            status(Status::initialized), availTest(nullptr)
        {}

        Status status;
        vtslibs::registry::BoundLayer::Availability *availTest;
    };

    CacheImpl(MapImpl *map, Fetcher *fetcher) : fetcher(fetcher), map(map),
        downloadingTasks(0)
    {
        if (!fetcher)
            return;
        Fetcher::Func func = std::bind(&CacheImpl::fetchedFile, this,
                                       std::placeholders::_1);
        fetcher->setCallback(func);
        
        try
        {
            Buffer buf = readLocalFileBuffer("cache/invalidUrl.txt");
            std::istringstream is(std::string((char*)buf.data, buf.size));
            while (is.good())
            {
                std::string line;
                std::getline(is, line);
                invalid.insert(line);
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
            for (auto it : invalid)
                fprintf(f, "%s\n", it.c_str());
            if (fclose(f) != 0)
                throw std::runtime_error("failed to write file");
        }
        catch (...)
        {
            // do nothing
        }
    }

    static const std::string convertNameToPath(std::string path,
                                               bool preserveSlashes)
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

    static const std::string convertNameToCache(const std::string &path)
    {
        uint32 p = path.find("://");
        std::string a = p == std::string::npos ? path : path.substr(p + 3);
        std::string b = boost::filesystem::path(a).parent_path().string();
        std::string c = a.substr(b.length() + 1);
        return std::string("cache/")
                + convertNameToPath(b, false) + "/"
                + convertNameToPath(c, false);
    }

    static Result result(Status status)
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
            b.allocate(b.size);
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
        CacheTask *t = tasks[name].get();
        try
        {
            t->contentData = readLocalFileBuffer(path);
            t->status = Status::ready;
        }
        catch (std::runtime_error &)
        {
            t->status = Status::error;
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

    void fetchedFile(FetchTask *task) override
    {
        downloadingTasks--;
        CacheTask *t = (CacheTask*)(task);
        
        // handle error codes
        if (t->code >= 400)
            t->status = Status::error;
        
        // availability tests
        if (t->status == Status::downloading)
        if (t->availTest)
        {
            switch (t->availTest->type)
            {
            case vtslibs::registry::BoundLayer
            ::Availability::Type::negativeCode:
                if (t->availTest->codes.find(t->code)
                        == t->availTest->codes.end())
                    t->status = Status::error;
                break;
            case vtslibs::registry::BoundLayer
            ::Availability::Type::negativeType:
                if (t->availTest->mime == t->contentType)
                    t->status = Status::error;
                break;
            case vtslibs::registry::BoundLayer
            ::Availability::Type::negativeSize:
                if (t->contentData.size <= t->availTest->size)
                    t->status = Status::error;
            default:
                throw std::runtime_error("invalid availability test type");
            }
        }
        
        // handle redirections
        if (t->status == Status::downloading)
        switch (task->code)
        {
        case 301:
        case 302:
        case 303:
        case 307:
        case 308:
            if (t->redirectionsCount++ > 5)
                t->status = Status::error;
            else
            {
                t->url = t->redirectUrl;
                fetcher->fetch(t);
                return;
            }
        }
        
        if (t->status == Status::error)
        {
            boost::lock_guard<boost::mutex> l(mut);
            invalidNew.insert(t->name);
            return;
        }
        
        try
        {
            writeLocalFile(convertNameToCache(t->name), t->contentData);
        }
        catch(std::runtime_error &)
        {
            // do nothing
        }
        
        t->status = Status::ready;
    }
    
    CacheTask *getTask(const std::string &name)
    {
        auto it = tasks.find(name);
        if (it != tasks.end())
            return it->second.get();
        return (tasks[name] = std::shared_ptr<CacheTask>(new CacheTask(name))).get();
    }

    Result read(const std::string &name, Buffer &buffer,
                bool allowDiskCache) override
    {
        if (invalid.find(name) != invalid.end())
            return result(Status::error);
        CacheTask *t = getTask(name);
        if (t->status == Status::initialized)
        {
            t->status = Status::downloading;
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
                    fetcher->fetch(t);
                    downloadingTasks++;
                    map->statistics->resourcesDownloaded++;
                }
                else
                    t->status = Status::initialized;
            }
        }
        if (t->status == Status::ready)
        {
            buffer = std::move(t->contentData);
            tasks.erase(t->name);
            return result(Status::ready);
        }
        return result(t->status);
    }
    
    void tick() override
    {
        boost::lock_guard<boost::mutex> l(mut);
        invalid.insert(invalidNew.begin(), invalidNew.end());
        invalidNew.clear();
    }
    
    std::unordered_map<std::string, std::shared_ptr<CacheTask>> tasks;
    std::unordered_set<std::string> invalid;
    std::unordered_set<std::string> invalidNew;
    boost::mutex mut;
    Fetcher *fetcher;
    MapImpl *map;
    boost::atomic<uint32> downloadingTasks;
};

Cache *Cache::create(MapImpl *map, Fetcher *fetcher)
{
    return new CacheImpl(map, fetcher);
}

Cache::~Cache()
{}

} // namespace melown
