#include <utility/path.hpp> // homeDir

#include "include/vts-browser/map.hpp"
#include "map.hpp"

namespace vts
{

namespace
{

template<class T>
std::shared_ptr<T> getMapResource(MapImpl *map, const std::string &name,
                                  FetchTask::ResourceType resourceType)
{
    auto it = map->resources.resources.find(name);
    if (it == map->resources.resources.end())
    {
        auto r = std::make_shared<T>();
        r->fetch = std::make_shared<FetchTaskImpl>(map, name, resourceType);
        map->resources.resources[name] = r;
        it = map->resources.resources.find(name);
    }
    map->touchResource(it->second);
    return std::dynamic_pointer_cast<T>(it->second);
}

} // namespace

Resource::Resource()
{}

Resource::~Resource()
{}

Resource::operator bool() const
{
    return fetch->state == FetchTaskImpl::State::ready;
}

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

void MapImpl::resourceDataInitialize(const std::shared_ptr<Fetcher> &fetcher)
{
    LOG(info3) << "Data initialize";
    assert(fetcher);
    resources.fetcher = fetcher;
    fetcher->initialize();
}

void MapImpl::resourceDataFinalize()
{
    LOG(info3) << "Data finalize";
    assert(resources.fetcher);
    resources.fetcher->finalize();
    resources.fetcher.reset();
    resources.prepareQueLocked.clear();
}

void MapImpl::loadResource(const std::shared_ptr<Resource> &resource)
{
    resource->info.gpuMemoryCost = resource->info.ramMemoryCost = 0;
    statistics.resourcesProcessLoaded++;
    try
    {
        resource->load();
    }
    catch (...)
    {
		resource->fetch->contentData.free();
		throw;
    }
    resource->fetch->contentData.free();
    resource->fetch->state = FetchTaskImpl::State::ready;
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
        ){ return a->fetch->priority > b->fetch->priority; });
    
    uint32 processed = 0;
    for (std::shared_ptr<Resource> r : res)
    {
        if (processed++ >= options.maxResourceProcessesPerTick)
            return false; // tasks left
        try
        {
			if (r->fetch->state == FetchTaskImpl::State::downloaded)
            {
				loadResource(r);
                continue;
            }
			else if (r->fetch->state != FetchTaskImpl::State::initializing)
				continue;
            if (resources.failedAvailUrlNoLock.find(r->fetch->name)
                    != resources.failedAvailUrlNoLock.end())
            {
                statistics.resourcesIgnored++;
                r->fetch->state = FetchTaskImpl::State::error;
				LOG(debug) << "Ignoring resource '"
						   << r->fetch->name
                           << "', becouse it is on failed-avail-test list";
            }
            else if (r->fetch->name.find("://") == std::string::npos)
            {
                r->fetch->loadFromInternalMemory();
                loadResource(r);
            }
            else if (r->fetch->allowDiskCache() && r->fetch->loadFromCache())
            {
                statistics.resourcesDiskLoaded++;
                loadResource(r);
            }
            else if (resources.downloads < options.maxConcurrentDownloads)
            {
                statistics.resourcesDownloaded++;
                r->fetch->state = FetchTaskImpl::State::downloading;
                r->fetch->replyCode = 0;
                resources.fetcher->fetch(r->fetch);
                resources.downloads++;
            }
        }
        catch (std::exception &e)
        {
			statistics.resourcesFailed++;
            r->fetch->state = FetchTaskImpl::State::error;
            LOG(err3) << "Failed processing resource '"
                      << r->fetch->name
                      << "', exception: " << e.what();
        }
    }
    
    return true; // all done
}

void MapImpl::fetchedFile(FetchTaskImpl *task)
{
    assert(task);
    LOG(debug) << "Fetched file '" << task->name << "'";
    task->state = FetchTaskImpl::State::downloading;
    
    // handle error or invalid codes
    if (task->replyCode >= 400 || task->replyCode < 200)
    {
        LOG(err3) << "Error downloading '"
                  << task->name
                  << "', http code " << task->replyCode;
        task->state = FetchTaskImpl::State::error;
    }
    
    // availability tests
    if (task->state == FetchTaskImpl::State::downloading)
    {
        if (!task->performAvailTest())
        {
            {
                boost::lock_guard<boost::mutex> l(resources.mutFailedAvailUrls);
                resources.failedAvailUrlLocked.insert(task->name);
            }
            LOG(debug) << "Availability test failed for task '"
                       << task->name << "'";
            task->state = FetchTaskImpl::State::error;
        }
    }
    
    resources.downloads--;
    
    // handle redirections
    if (task->state == FetchTaskImpl::State::downloading
            && task->replyCode >= 300 && task->replyCode < 400)
    {
        if (task->redirectionsCount++ > 5)
        {
            LOG(err3) << "Too many redirections in '"
                      << task->name << "', last url '"
                      << task->queryUrl << "', http code " << task->replyCode;
            task->state = FetchTaskImpl::State::error;
        }
        else
        {
            task->queryUrl.swap(task->replyRedirectUrl);
            task->replyRedirectUrl = "";
            LOG(debug) << "task '"
                       << task->name << "' redirected to '"
                       << task->queryUrl << "', http code " << task->replyCode;
            task->replyCode = 0;
            task->state = FetchTaskImpl::State::initializing;
            return;
        }
    }
    
    if (task->state == FetchTaskImpl::State::error)
    {
        task->contentData.free();
        return;
    }
    
    if (!resources.disableCache)
        task->saveToCache();
    task->state = FetchTaskImpl::State::downloaded;
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
            memRamUse += it.second->info.ramMemoryCost;
            memGpuUse += it.second->info.gpuMemoryCost;
            // consider long time not used resources only
            if (it.second->fetch->lastAccessTick + 100 < statistics.frameIndex)
                res.push_back(it.second.get());
        }
        uint64 memUse = memRamUse + memGpuUse;
        if (memUse > options.maxResourcesMemory)
        {
            std::sort(res.begin(), res.end(), [](Resource *a, Resource *b){
                //if (a->impl->lastAccessTick == b->impl->lastAccessTick)
                //    return a->gpuMemoryCost + a->ramMemoryCost
                //            > b->gpuMemoryCost + b->ramMemoryCost;
                return a->fetch->lastAccessTick < b->fetch->lastAccessTick;
            });
            for (Resource *it : res)
            {
                if (memUse <= options.maxResourcesMemory)
                    break;
                memUse -= it->info.gpuMemoryCost + it->info.ramMemoryCost;
                if (it->fetch->state != FetchTaskImpl::State::finalizing)
                    it->fetch->state = FetchTaskImpl::State::finalizing;
                else
                {
                    statistics.resourcesReleased++;
                    LOG(info2) << "Releasing resource '"
                               << it->fetch->name << "'";
                    resources.resources.erase(it->fetch->name);
                }
            }
        }
        statistics.currentGpuMemUse = memGpuUse;
        statistics.currentRamMemUse = memRamUse;
    }
    statistics.currentResources = resources.resources.size();
}

void MapImpl::touchResource(const std::shared_ptr<Resource> &resource)
{
    touchResource(resource, resource->fetch->priority);
}

void MapImpl::touchResource(const std::shared_ptr<Resource> &resource,
                            double priority)
{
    resource->fetch->lastAccessTick = statistics.frameIndex;
    resource->fetch->priority = priority;
    switch (resource->fetch->state)
    {
    case FetchTaskImpl::State::finalizing:
        resource->fetch->state = FetchTaskImpl::State::initializing;
        // no break here
    case FetchTaskImpl::State::initializing:
    case FetchTaskImpl::State::downloaded:
        resources.prepareQueNoLock.insert(resource);
        break;
    case FetchTaskImpl::State::downloading:
    case FetchTaskImpl::State::ready:
    case FetchTaskImpl::State::error:
        break;
    }
}

std::shared_ptr<GpuTexture> MapImpl::getTexture(const std::string &name)
{
    return getMapResource<GpuTexture>(this, name,
                                    FetchTask::ResourceType::Texture);
}

std::shared_ptr<GpuMesh> MapImpl::getMeshRenderable(const std::string &name)
{
    return getMapResource<GpuMesh>(this, name,
                                    FetchTask::ResourceType::MeshPart);
}

std::shared_ptr<AuthConfig> MapImpl::getAuth(const std::string &name)
{
    return getMapResource<AuthConfig>(this, name,
                                    FetchTask::ResourceType::AuthConfig);
}

std::shared_ptr<MapConfig> MapImpl::getMapConfig(const std::string &name)
{
    return getMapResource<MapConfig>(this, name,
                                     FetchTask::ResourceType::MapConfig);
}

std::shared_ptr<MetaTile> MapImpl::getMetaTile(const std::string &name)
{
    return getMapResource<MetaTile>(this, name,
                                    FetchTask::ResourceType::MetaTile);
}

std::shared_ptr<NavTile> MapImpl::getNavTile(
        const std::string &name)
{
    return getMapResource<NavTile>(this, name,
                                   FetchTask::ResourceType::NavTile);
}

std::shared_ptr<MeshAggregate> MapImpl::getMeshAggregate(
        const std::string &name)
{
    return getMapResource<MeshAggregate>(this, name,
                                         FetchTask::ResourceType::Mesh);
}

std::shared_ptr<ExternalBoundLayer> MapImpl::getExternalBoundLayer(
        const std::string &name)
{
    return getMapResource<ExternalBoundLayer>(this, name,
                    FetchTask::ResourceType::BoundLayerConfig);
}

std::shared_ptr<BoundMetaTile> MapImpl::getBoundMetaTile(
        const std::string &name)
{
    return getMapResource<BoundMetaTile>(this, name,
                    FetchTask::ResourceType::BoundMetaTile);
}

std::shared_ptr<BoundMaskTile> MapImpl::getBoundMaskTile(
        const std::string &name)
{
    return getMapResource<BoundMaskTile>(this, name,
                    FetchTask::ResourceType::BoundMaskTile);
}

Validity MapImpl::getResourceValidity(const std::string &name)
{
    auto it = resources.resources.find(name);
    if (it == resources.resources.end())
        return Validity::Invalid;
    switch (it->second->fetch->state)
    {
    case FetchTaskImpl::State::error:
        return Validity::Invalid;
    case FetchTaskImpl::State::finalizing:
    case FetchTaskImpl::State::initializing:
    case FetchTaskImpl::State::downloading:
    case FetchTaskImpl::State::downloaded:
        return Validity::Indeterminate;
    case FetchTaskImpl::State::ready:
        return Validity::Valid;
    default:
        LOGTHROW(fatal, std::invalid_argument) << "Invalid resource state";
    }
    throw; // shut up compiler warning
}

const std::string MapImpl::Resources::failedAvailTestFileName
= "failedAvailTestUrls.txt";

ResourceInfo::ResourceInfo() :
    ramMemoryCost(0), gpuMemoryCost(0)
{}

} // namespace vts
