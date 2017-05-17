#include <utility/path.hpp> // homeDir
#include <vts/map.hpp>

#include "map.hpp"

namespace vts
{

namespace
{

template<class T>
std::shared_ptr<T> getMapResource(const std::string &name, MapImpl *map,
                                  double priority = 0)
{
    auto it = map->resources.resources.find(name);
    if (it == map->resources.resources.end())
    {
        map->resources.resources[name] = std::make_shared<T>(name);
        it = map->resources.resources.find(name);
    }
    map->touchResource(it->second, priority);
    return std::dynamic_pointer_cast<T>(it->second);
}

} // namespace

MapImpl::Resources::Resources(const MapCreateOptions &options)
    : cachePath(options.cachePath), downloads(0), fetcher(nullptr),
      disableCache(options.disableCache)
{
    if (!disableCache)
    {
        if (cachePath.empty())
        {
            cachePath = utility::homeDir().string();
            if (cachePath.empty())
                LOGTHROW(err3, std::runtime_error)
                        << "invalid home dir, the cache path must be defined";
            cachePath += "/.cache/vts-browser/";
        }
        if (cachePath.back() != '/')
            cachePath += "/";
        try
        {
            if (boost::filesystem::exists(cachePath + failedAvailTestFileName))
            {
                std::ifstream f;
                f.open(cachePath + failedAvailTestFileName);
                while (f.good())
                {
                    std::string line;
                    std::getline(f, line);
                    failedAvailUrlNoLock.insert(line);
                }
                f.close();
            }
        }
        catch (...)
        {
            // do nothing
        }
    }
}

MapImpl::Resources::~Resources()
{
    if (!disableCache)
    {
        try
        {
            std::ofstream f;
            f.open(cachePath + failedAvailTestFileName);
            for (auto &&line : failedAvailUrlNoLock)
                f << line << '\n';
            f.close();
        }
        catch (...)
        {
            // do nothing
        }
    }
}

void MapImpl::resourceDataInitialize(Fetcher *fetcher)
{
    LOG(info3) << "Data initialize";
    assert(fetcher);
    resources.fetcher = fetcher;
    Fetcher::Func func = std::bind(
                &MapImpl::fetchedFile,
                this, std::placeholders::_1);
    fetcher->initialize(func);
}

void MapImpl::resourceDataFinalize()
{
    LOG(info3) << "Data finalize";
    assert(resources.fetcher);
    resources.fetcher->finalize();
    resources.fetcher = nullptr;
    resources.prepareQueLocked.clear();
}

void MapImpl::loadResource(std::shared_ptr<Resource> r)
{
    statistics.resourcesProcessLoaded++;
    try
    {
        r->load(this);
    }
    catch (...)
    {
		r->impl->contentData.free();
		throw;
    }
    r->impl->contentData.free();
    r->impl->state = ResourceImpl::State::ready;
}

bool MapImpl::resourceDataTick()
{    
    statistics.currentResourceDownloads = resources.downloads;
    
    { // sync failed avail urls
        boost::lock_guard<boost::mutex> l(resources.mutFailedAvailUrls);
        resources.failedAvailUrlNoLock.insert(
                    resources.failedAvailUrlLocked.begin(),
                    resources.failedAvailUrlLocked.end());
        resources.failedAvailUrlLocked.clear();
    }
    
    std::vector<std::shared_ptr<Resource>> res;
    { // sync resources
        boost::lock_guard<boost::mutex> l(resources.mutPrepareQue);
        if (resources.prepareQueLocked.empty())
            return true; // all done
        res.reserve(resources.prepareQueLocked.size());
        for (std::shared_ptr<Resource> r : resources.prepareQueLocked)
            res.push_back(r);
    }
    
    // sort resources by priority
    std::sort(res.begin(), res.end(), [](
              std::shared_ptr<Resource> a,
              std::shared_ptr<Resource> b
        ){ return a->impl->priority > b->impl->priority; });
    
    uint32 processed = 0;
    for (std::shared_ptr<Resource> r : res)
    {
        if (processed++ >= options.maxResourceProcessesPerTick)
            return false; // tasks left
        try
        {
			if (r->impl->state == ResourceImpl::State::downloaded)
            {
				loadResource(r);
                continue;
            }
			else if (r->impl->state != ResourceImpl::State::initializing)
				continue;
            if (resources.failedAvailUrlNoLock.find(r->impl->name)
                    != resources.failedAvailUrlNoLock.end())
            {
                statistics.resourcesIgnored++;
                r->impl->state = ResourceImpl::State::error;
				LOG(debug) << "Ignoring resource '"
						   << r->impl->name
                           << "', becouse it is on failed-avail-test list";
            }
            else if (r->impl->name.find("://") == std::string::npos)
            {
                r->impl->loadFromInternalMemory();
                loadResource(r);
            }
            else if (!resources.disableCache && r->impl->allowDiskCache
                     && r->impl->loadFromCache(this))
            {
                statistics.resourcesDiskLoaded++;
                loadResource(r);
            }
            else if (resources.downloads < options.maxConcurrentDownloads)
            {
                statistics.resourcesDownloaded++;
                r->impl->state = ResourceImpl::State::downloading;
                r->impl->queryHeaders.clear();
                if (auth)
                    auth->authorize(r);
                resources.fetcher->fetch(r->impl);
                resources.downloads++;
            }
        }
        catch (std::exception &e)
        {
			statistics.resourcesFailed++;
            r->impl->state = ResourceImpl::State::error;
            LOG(err3) << "Failed processing resource '"
                      << r->impl->name
                      << "', exception: " << e.what();
        }
    }
    
    return true; // all done
}

void MapImpl::fetchedFile(std::shared_ptr<FetchTask> task)
{
    std::shared_ptr<ResourceImpl> resource
            = std::dynamic_pointer_cast<ResourceImpl>(task);
    assert(resource);
    LOG(debug) << "Fetched file '" << resource->name << "'";
    resource->state = ResourceImpl::State::downloading;
    
    // handle error or invalid codes
    if (resource->replyCode >= 400 || resource->replyCode < 200)
    {
        LOG(err3) << "Error downloading '"
                  << resource->name
                  << "', http code " << resource->replyCode;
        resource->state = ResourceImpl::State::error;
    }
    
    // availability tests
    if (resource->state == ResourceImpl::State::downloading)
    {
        if (!resource->performAvailTest())
        {
            {
                boost::lock_guard<boost::mutex> l(resources.mutFailedAvailUrls);
                resources.failedAvailUrlLocked.insert(resource->name);
            }
            LOG(debug) << "Availability test failed for resource '"
                       << resource->name << "'";
            resource->state = ResourceImpl::State::error;
        }
    }
    
    // handle redirections
    if (resource->state == ResourceImpl::State::downloading
            && resource->replyCode >= 300 && resource->replyCode < 400)
    {
        if (resource->redirectionsCount++ > 5)
        {
            LOG(err3) << "Too many redirections in '"
                      << resource->name << "', last url '"
                      << resource->queryUrl << "', http code " << resource->replyCode;
            resource->state = ResourceImpl::State::error;
        }
        else
        {
            resource->queryUrl.swap(resource->replyRedirectUrl);
            resource->replyRedirectUrl = "";
            LOG(debug) << "Resource '"
                       << resource->name << "' redirected to '"
                       << resource->queryUrl << "', http code " << resource->replyCode;
            resources.fetcher->fetch(task);
            // do not decrease the downloads counter here
            return;
        }
    }
    
    resources.downloads--;
    
    if (resource->state == ResourceImpl::State::error)
    {
        resource->contentData.free();
        return;
    }
    
    if (!resources.disableCache)
        resource->saveToCache(this);
    resource->state = ResourceImpl::State::downloaded;
}

void MapImpl::resourceRenderInitialize()
{}

void MapImpl::resourceRenderFinalize()
{
    resources.prepareQueNoLock.clear();
    resources.resources.clear();
}

void MapImpl::resourceRenderTick()
{
    statistics.currentResourcePreparing = resources.prepareQueNoLock.size()
            + statistics.currentResourceDownloads;
    { // sync download queue
        boost::lock_guard<boost::mutex> l(resources.mutPrepareQue);
        std::swap(resources.prepareQueNoLock, resources.prepareQueLocked);
    }
    resources.prepareQueNoLock.clear();
    
    // clear old resources
    if (statistics.frameIndex % 31 == 0)
    {
        std::vector<Resource*> res;
        res.reserve(resources.resources.size());
        uint64 memRamUse = 0;
        uint64 memGpuUse = 0;
        for (auto &&it : resources.resources)
        {
            memRamUse += it.second->impl->ramMemoryCost;
            memGpuUse += it.second->impl->gpuMemoryCost;
            // consider long time not used resources only
            if (it.second->impl->lastAccessTick + 100 < statistics.frameIndex)
                res.push_back(it.second.get());
        }
        uint64 memUse = memRamUse + memGpuUse;
        if (memUse > options.maxResourcesMemory)
        {
            std::sort(res.begin(), res.end(), [](Resource *a, Resource *b){
                //if (a->impl->lastAccessTick == b->impl->lastAccessTick)
                //    return a->gpuMemoryCost + a->ramMemoryCost
                //            > b->gpuMemoryCost + b->ramMemoryCost;
                return a->impl->lastAccessTick < b->impl->lastAccessTick;
            });
            for (Resource *it : res)
            {
                if (memUse <= options.maxResourcesMemory)
                    break;
                memUse -= it->impl->gpuMemoryCost + it->impl->ramMemoryCost;
                if (it->impl->state != ResourceImpl::State::finalizing)
                    it->impl->state = ResourceImpl::State::finalizing;
                else
                {
                    statistics.resourcesReleased++;
                    LOG(info2) << "Releasing resource '" << it->impl->name << "'";
                    resources.resources.erase(it->impl->name);
                }
            }
        }
        statistics.currentGpuMemUse = memGpuUse;
        statistics.currentRamMemUse = memRamUse;
    }
    statistics.currentResources = resources.resources.size();
}

void MapImpl::touchResource(std::shared_ptr<Resource> resource)
{
    touchResource(resource, resource->impl->priority);
}

void MapImpl::touchResource(std::shared_ptr<Resource> resource,
                            double priority)
{
    resource->impl->lastAccessTick = statistics.frameIndex;
    resource->impl->priority = priority;
    switch (resource->impl->state)
    {
    case ResourceImpl::State::finalizing:
        resource->impl->state = ResourceImpl::State::initializing;
        // no break here
    case ResourceImpl::State::initializing:
    case ResourceImpl::State::downloaded:
        resources.prepareQueNoLock.insert(resource);
        break;
    case ResourceImpl::State::downloading:
    case ResourceImpl::State::ready:
    case ResourceImpl::State::error:
        break;
    }
}

std::shared_ptr<GpuTexture> MapImpl::getTexture(const std::string &name)
{
    auto it = resources.resources.find(name);
    if (it == resources.resources.end())
    {
        resources.resources[name] = callbacks.createTexture(name);
        it = resources.resources.find(name);
    }
    touchResource(it->second);
    return std::dynamic_pointer_cast<GpuTexture>(it->second);
}

std::shared_ptr<GpuMesh> MapImpl::getMeshRenderable(const std::string &name)
{
    auto it = resources.resources.find(name);
    if (it == resources.resources.end())
    {
        resources.resources[name] = callbacks.createMesh(name);
        it = resources.resources.find(name);
    }
    touchResource(it->second);
    return std::dynamic_pointer_cast<GpuMesh>(it->second);
}

std::shared_ptr<AuthJson> MapImpl::getAuth(const std::string &name)
{
    return getMapResource<AuthJson>(name, this,
                    std::numeric_limits<double>::infinity());
}

std::shared_ptr<MapConfig> MapImpl::getMapConfig(const std::string &name)
{
    return getMapResource<MapConfig>(name, this,
                    std::numeric_limits<double>::infinity());
}

std::shared_ptr<MetaTile> MapImpl::getMetaTile(const std::string &name)
{
    return getMapResource<MetaTile>(name, this);
}

std::shared_ptr<NavTile> MapImpl::getNavTile(
        const std::string &name)
{
    return getMapResource<NavTile>(name, this);
}

std::shared_ptr<MeshAggregate> MapImpl::getMeshAggregate(
        const std::string &name)
{
    return getMapResource<MeshAggregate>(name, this);
}

std::shared_ptr<ExternalBoundLayer> MapImpl::getExternalBoundLayer(
        const std::string &name)
{
    return getMapResource<ExternalBoundLayer>(name, this,
                    std::numeric_limits<double>::infinity());
}

std::shared_ptr<BoundMetaTile> MapImpl::getBoundMetaTile(
        const std::string &name)
{
    return getMapResource<BoundMetaTile>(name, this);
}

std::shared_ptr<BoundMaskTile> MapImpl::getBoundMaskTile(
        const std::string &name)
{
    return getMapResource<BoundMaskTile>(name, this);
}

Validity MapImpl::getResourceValidity(const std::string &name)
{
    auto it = resources.resources.find(name);
    if (it == resources.resources.end())
        return Validity::Invalid;
    switch (it->second->impl->state)
    {
    case ResourceImpl::State::error:
        return Validity::Invalid;
    case ResourceImpl::State::finalizing:
    case ResourceImpl::State::initializing:
    case ResourceImpl::State::downloading:
    case ResourceImpl::State::downloaded:
        return Validity::Indeterminate;
    case ResourceImpl::State::ready:
        return Validity::Valid;
    default:
        LOGTHROW(fatal, std::invalid_argument) << "Invalid resource state";
    }
    throw; // shut up compiler warning
}

const std::string MapImpl::Resources::failedAvailTestFileName
                        = "failedAvailTestUrls.txt";

} // namespace vts
