#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <atomic>

#include <boost/thread/mutex.hpp>

#include <renderer/gpuContext.h>
#include <renderer/gpuResources.h>
#include <renderer/statistics.h>

#include "map.h"
#include "resource.h"
#include "mapConfig.h"
#include "mapResources.h"
#include "resourceManager.h"

namespace melown
{

namespace
{

class ResourceManagerImpl : public ResourceManager
{
public:
    /////////
    /// DATA thread
    /////////

    static const std::string invalidurlFileName;
    
    ResourceManagerImpl(MapImpl *map) : map(map),
        takeItemIndex(0), tickIndex(0), downloads(0)
    {
        try
        {
            if (boost::filesystem::exists(invalidurlFileName))
            {
                std::ifstream f;
                f.open(invalidurlFileName);
                while (f.good())
                {
                    std::string line;
                    std::getline(f, line);
                    invalidUrl.insert(line);
                }
                f.close();
            }
        }
        catch (...)
        {}
    }

    ~ResourceManagerImpl()
    {
        try
        {
            std::ofstream f;
            f.open(invalidurlFileName);
            for (auto &&line : invalidUrl)
                f << line << '\n';
            f.close();
        }
        catch (...)
        {}
    }

    void dataInitialize(GpuContext *context, Fetcher *fetcher) override
    {
        dataContext = context;
        this->fetcher = fetcher;
        Fetcher::Func func = std::bind(
                    &ResourceManagerImpl::fetchedFile,
                    this, std::placeholders::_1);
        fetcher->initialize(func);
    }
    
    void dataFinalize() override
    {
        dataContext = nullptr;
        fetcher->finalize();
        downloadQue.clear();
    }
    
    void loadResource(ResourceImpl *r)
    {
        assert(r->state == ResourceImpl::State::preparing);
        assert(r->download);
        
        try
        {
            r->resource->load(map);
            r->state = ResourceImpl::State::ready;
        }
        catch (std::runtime_error &)
        {
            r->state = ResourceImpl::State::errorLoad;
        }
        r->download.reset();
    }

    bool dataTick() override
    {
        { // sync invalid urls
            boost::lock_guard<boost::mutex> l(mutInvalidUrls);
            invalidUrl.insert(invalidUrlNew.begin(), invalidUrlNew.end());
            invalidUrlNew.clear();
        }
        
        { // load resource
            ResourceImpl *r = nullptr;
            {
                boost::lock_guard<boost::mutex> l(mutLoadQue);
                if (!loadQue.empty())
                {
                    r = loadQue.front();
                    loadQue.pop();
                }
            }
            if (r)
            {
                loadResource(r);
                return false;
            }
        }
        
        if (downloads.load() < 5)
        { // download resource
            ResourceImpl *r = nullptr;
            {
                boost::lock_guard<boost::mutex> l(mutDownloadQue);
                if (!downloadQue.empty())
                {
                    auto it = std::next(downloadQue.begin(),
                                        takeItemIndex++ % downloadQue.size());
                    r = *it;
                    downloadQue.erase(it);
                }
            }
            if (r && r->state == ResourceImpl::State::initializing)
            {
                if (invalidUrl.find(r->resource->name) != invalidUrl.end())
                {
                    r->state = ResourceImpl::State::errorLoad;
                    return false;
                }
                
                r->state = ResourceImpl::State::preparing;
                
                assert(!r->download);
                r->download.emplace(r);
                
                if (r->resource->name.find("://") == std::string::npos)
                {
                    r->download->readLocalFile();
                    loadResource(r);
                }
                else if (r->download->loadFromCache())
                {
                    map->statistics->resourcesDiskLoaded++;
                    loadResource(r);
                }
                else
                {
                    fetcher->fetch(r->download.get_ptr());
                    map->statistics->resourcesDownloaded++;
                    downloads++;
                }
                
                return false;
            }
        }
        
        // sleep
        return true;
    }
    
    
    /////////
    /// DOWNLOAD thread
    /////////
    
    
    void fetchedFile(FetchTask *task)
    {
        ResourceImpl *resource = ((ResourceImpl::DownloadTask*)task)->resource;
        assert(resource->state == ResourceImpl::State::preparing);
        assert(resource->download);
        
        // handle error codes
        if (task->code >= 400 || task->code == 0)
            resource->state = ResourceImpl::State::errorDownload;
        
        // availability tests
        if (resource->state == ResourceImpl::State::initializing)
        if (resource->availTest)
        {
            switch (resource->availTest->type)
            {
            case vtslibs::registry::BoundLayer
            ::Availability::Type::negativeCode:
                if (resource->availTest->codes.find(task->code)
                        == resource->availTest->codes.end())
                    resource->state = ResourceImpl::State::errorDownload;
                break;
            case vtslibs::registry::BoundLayer
            ::Availability::Type::negativeType:
                if (resource->availTest->mime == task->contentType)
                    resource->state = ResourceImpl::State::errorDownload;
                break;
            case vtslibs::registry::BoundLayer
            ::Availability::Type::negativeSize:
                if (task->contentData.size <= resource->availTest->size)
                    resource->state = ResourceImpl::State::errorDownload;
            default:
                throw std::invalid_argument("invalid availability test type");
            }
        }
        
        // handle redirections
        if (resource->state == ResourceImpl::State::initializing)
        switch (task->code)
        {
        case 301:
        case 302:
        case 303:
        case 307:
        case 308:
            if (task->redirectionsCount++ > 5)
                resource->state = ResourceImpl::State::errorDownload;
            else
            {
                task->url = task->redirectUrl;
                fetcher->fetch(task);
                return;
            }
        }
        
        downloads--;
        
        if (resource->state == ResourceImpl::State::errorDownload)
        {
            resource->download.reset();
            boost::lock_guard<boost::mutex> l(mutInvalidUrls);
            invalidUrlNew.insert(resource->resource->name);
            return;
        }
        
        ((ResourceImpl::DownloadTask*)task)->saveToCache();
        {
            boost::lock_guard<boost::mutex> l(mutLoadQue);
            loadQue.push(resource);
        }
    }
    
    
    /////////
    /// RENDER thread
    /////////

    
    void renderInitialize(GpuContext *context) override
    {
        renderContext = context;
    }
    
    void renderFinalize() override
    {
        renderContext = nullptr;
        downloadQueNew.clear();
        resources.clear();
    }

    void renderTick() override
    {
        { // sync download queue
            boost::lock_guard<boost::mutex> l(mutDownloadQue);
            std::swap(downloadQueNew, downloadQue);
        }
        downloadQueNew.clear();
        
        // clear old resources
        if (tickIndex % 10 == 0)
        {
            std::vector<Resource*> res;
            res.reserve(resources.size());
            uint32 memUse = 0;
            for (auto &&it : resources)
            {
                memUse += it.second->gpuMemoryCost + it.second->ramMemoryCost;
                // consider long time not used resources only
                if (it.second->impl->lastAccessTick + 100 < tickIndex
                        && it.second->impl->download)
                    res.push_back(it.second.get());
            }
            static const uint32 memCap = 500000000;
            if (memUse > memCap)
            {
                std::sort(res.begin(), res.end(), [](Resource *a, Resource *b){
                    if (a->impl->lastAccessTick == b->impl->lastAccessTick)
                        return a->gpuMemoryCost + a->ramMemoryCost
                                > b->gpuMemoryCost + b->ramMemoryCost;
                    return a->impl->lastAccessTick < b->impl->lastAccessTick;
                });
                for (Resource *it : res)
                {
                    if (memUse <= memCap)
                        break;
                    memUse -= it->gpuMemoryCost + it->ramMemoryCost;
                    if (it->impl->state != ResourceImpl::State::finalizing)
                        it->impl->state = ResourceImpl::State::finalizing;
                    else
                        resources.erase(it->name);
                }
            }
        }
        
        // advance the tick counter
        tickIndex++;
    }

    void touch(const std::string &, Resource *resource)
    {
        resource->impl->lastAccessTick = tickIndex;
        switch (resource->impl->state)
        {
        case ResourceImpl::State::finalizing:
            resource->impl->state = ResourceImpl::State::initializing;
            // intentionally no break here
        case ResourceImpl::State::initializing:
            downloadQueNew.insert(resource->impl.get());
            break;
        }
    }

    template<class T,
             std::shared_ptr<Resource>(GpuContext::*F)(const std::string &)>
    T *getGpuResource(const std::string &name)
    {
        auto it = resources.find(name);
        if (it == resources.end())
        {
            resources[name] = (renderContext->*F)(name);
            it = resources.find(name);
        }
        touch(name, it->second.get());
        return dynamic_cast<T*>(it->second.get());
    }

    GpuShader *getShader(const std::string &name) override
    {
        return getGpuResource<GpuShader, &GpuContext::createShader>(name);
    }

    GpuTexture *getTexture(const std::string &name) override
    {
        return getGpuResource<GpuTexture, &GpuContext::createTexture>(name);
    }

    GpuMeshRenderable *getMeshRenderable(const std::string &name) override
    {
        return getGpuResource<GpuMeshRenderable,
                &GpuContext::createMeshRenderable>(name);
    }

    template<class T> T *getMapResource(const std::string &name)
    {
        auto it = resources.find(name);
        if (it == resources.end())
        {
            resources[name] = std::shared_ptr<Resource>(new T(name));
            it = resources.find(name);
        }
        touch(name, it->second.get());
        return dynamic_cast<T*>(it->second.get());
    }

    MapConfig *getMapConfig(const std::string &name) override
    {
        return getMapResource<MapConfig>(name);
    }

    MetaTile *getMetaTile(const std::string &name) override
    {
        return getMapResource<MetaTile>(name);
    }

    MeshAggregate *getMeshAggregate(const std::string &name) override
    {
        return getMapResource<MeshAggregate>(name);
    }

    ExternalBoundLayer *getExternalBoundLayer(const std::string &name) override
    {
        return getMapResource<ExternalBoundLayer>(name);
    }

    BoundMetaTile *getBoundMetaTile(const std::string &name) override
    {
        return getMapResource<BoundMetaTile>(name);
    }

    BoundMaskTile *getBoundMaskTile(const std::string &name) override
    {
        return getMapResource<BoundMaskTile>(name);
    }
    
    bool ready(const std::string &name) override
    {
        auto &&it = resources.find(name);
        if (it == resources.end())
            return false;
        return *it->second;
    }

    std::unordered_map<std::string, std::shared_ptr<Resource>> resources;
    std::unordered_set<ResourceImpl*> downloadQue;
    std::unordered_set<ResourceImpl*> downloadQueNew;
    std::queue<ResourceImpl*> loadQue;
    std::unordered_set<std::string> invalidUrl;
    std::unordered_set<std::string> invalidUrlNew;
    boost::mutex mutDownloadQue;
    boost::mutex mutLoadQue;
    boost::mutex mutInvalidUrls;
    MapImpl *map;
    Fetcher *fetcher;
    uint32 takeItemIndex;
    uint32 tickIndex;
    std::atomic_uint downloads;
};

const std::string ResourceManagerImpl::invalidurlFileName
            = "cache/invalidUrl.txt";

} // anonymous namespace

ResourceManager::ResourceManager() : renderContext(nullptr),
    dataContext(nullptr)
{}

ResourceManager::~ResourceManager()
{}

ResourceManager *ResourceManager::create(MapImpl *map)
{
    return new ResourceManagerImpl(map);
}

} // namespace melown
