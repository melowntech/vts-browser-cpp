#include <utility/path.hpp>
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

MapImpl::Resources::Resources(const std::string &cachePathVal,
                              bool keepInvalidUrls)
    : downloads(0), cachePath(cachePathVal),
    destroyTheFetcher(false), fetcher(nullptr)
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
        if (keepInvalidUrls && boost::filesystem::exists(
                    cachePath + invalidUrlFileName))
        {
            std::ifstream f;
            f.open(cachePath + invalidUrlFileName);
            while (f.good())
            {
                std::string line;
                std::getline(f, line);
                invalidUrlNoLock.insert(line);
            }
            f.close();
        }
    }
    catch (...)
    {
		// do nothing
	}
}

MapImpl::Resources::~Resources()
{
    if (destroyTheFetcher)
    {
        delete fetcher;
        fetcher = nullptr;
        destroyTheFetcher = false;
    }
    try
    {
        std::ofstream f;
        f.open(cachePath + invalidUrlFileName);
        for (auto &&line : invalidUrlNoLock)
            f << line << '\n';
        f.close();
    }
    catch (...)
    {
		// do nothing
	}
}

void MapImpl::dataInitialize(Fetcher *fetcher)
{
    LOG(info3) << "Data initialize";
    if (!fetcher)
    {
        resources.destroyTheFetcher = true;
        fetcher = Fetcher::create();
    }
    resources.fetcher = fetcher;
    Fetcher::Func func = std::bind(
                &MapImpl::fetchedFile,
                this, std::placeholders::_1);
    fetcher->initialize(func);
}

void MapImpl::dataFinalize()
{
    LOG(info3) << "Data finalize";
    assert(resources.fetcher);
    resources.fetcher->finalize();
    resources.prepareQueLocked.clear();
}

void MapImpl::loadResource(ResourceImpl *r)
{
    statistics.resourcesProcessLoaded++;
    try
    {
        r->resource->load(this);
    }
    catch (...)
    {
		r->contentData.free();
		throw;
    }
    r->contentData.free();
    r->state = ResourceImpl::State::ready;
}

bool MapImpl::dataTick()
{    
    statistics.currentResourceDownloads = resources.downloads;
    
    { // sync invalid urls
        boost::lock_guard<boost::mutex> l(resources.mutInvalidUrls);
        resources.invalidUrlNoLock.insert(resources.invalidUrlLocked.begin(),
                                    resources.invalidUrlLocked.end());
        resources.invalidUrlLocked.clear();
    }
    
    std::vector<ResourceImpl*> res;
    { // sync resources
        boost::lock_guard<boost::mutex> l(resources.mutPrepareQue);
        if (resources.prepareQueLocked.empty())
            return true; // all done
        res.reserve(resources.prepareQueLocked.size());
        for (ResourceImpl *r : resources.prepareQueLocked)
            res.push_back(r);
    }
    
    // sort resources by priority
    std::sort(res.begin(), res.end(), [](ResourceImpl *a, ResourceImpl *b){
        return a->priority > b->priority;
    });
    
    uint32 processed = 0;
    for (ResourceImpl *r : res)
    {
        if (processed++ >= options.maxResourceProcessesPerTick)
            return false; // tasks left
        try
        {
			if (r->state == ResourceImpl::State::downloaded)
            {
				loadResource(r);
                continue;
            }
			else if (r->state != ResourceImpl::State::initializing)
				continue;
            if (resources.invalidUrlNoLock.find(r->resource->name)
                    != resources.invalidUrlNoLock.end())
            {
                statistics.resourcesIgnored++;
                r->state = ResourceImpl::State::error;
				LOG(warn2) << "Ignoring resource '"
						   << r->resource->name << "'";
            }
            else if (r->resource->name.find("://") == std::string::npos)
            {
                r->state = ResourceImpl::State::downloading;
                r->loadFromInternalMemory();
                loadResource(r);
            }
            else if (r->resource->name.find(".json") == std::string::npos
                     && availableInCache(r->resource->name))
            {
                statistics.resourcesDiskLoaded++;
                r->state = ResourceImpl::State::downloading;
                r->loadFromCache(this);
                loadResource(r);
            }
            else if (resources.downloads < options.maxConcurrentDownloads)
            {
                statistics.resourcesDownloaded++;
                r->state = ResourceImpl::State::downloading;
                resources.fetcher->fetch(r);
                resources.downloads++;
            }
        }
        catch (std::exception &)
        {
			statistics.resourcesFailed++;
            r->state = ResourceImpl::State::error;
            LOG(err3) << "Failed processing resource '"
                      << r->resource->name << "'";
        }
    }
    
    return true; // all done
}

void MapImpl::fetchedFile(FetchTask *task)
{
    ResourceImpl *resource = dynamic_cast<ResourceImpl*>(task);
    LOG(debug) << "Fetched file '" << resource->resource->name << "'";
    assert(resource->state == ResourceImpl::State::downloading);
    
    // handle error codes
    if (task->code >= 400 || task->code == 0)
    {
        LOG(err3) << "Error downloading '"
                  << resource->resource->name << "', last url '"
                  << task->url << "', http code " << task->code;
        resource->state = ResourceImpl::State::error;
    }
    
    // availability tests
    if (resource->state == ResourceImpl::State::downloading
            && resource->availTest)
    {
        switch (resource->availTest->type)
        {
        case vtslibs::registry::BoundLayer
        ::Availability::Type::negativeCode:
            if (resource->availTest->codes.find(task->code)
                    == resource->availTest->codes.end())
                resource->state = ResourceImpl::State::error;
            break;
        case vtslibs::registry::BoundLayer
        ::Availability::Type::negativeType:
            if (resource->availTest->mime == task->contentType)
                resource->state = ResourceImpl::State::error;
            break;
        case vtslibs::registry::BoundLayer
        ::Availability::Type::negativeSize:
            if (task->contentData.size() <= resource->availTest->size)
                resource->state = ResourceImpl::State::error;
        default:
			assert(false);
        }
        if (resource->state == ResourceImpl::State::error)
        {
            LOG(debug) << "Availability test failed for resource '"
                       << resource->resource->name << "'";
        }
    }
    
    // handle redirections
    if (resource->state == ResourceImpl::State::downloading)
    {
        switch (task->code)
        {
        case 301:
        case 302:
        case 303:
        case 307:
        case 308:
            if (task->redirectionsCount++ > 5)
            {
                LOG(err3) << "Too many redirections in '"
                          << resource->resource->name << "', last url '"
                          << task->url << "', http code " << task->code;
                resource->state = ResourceImpl::State::error;
            }
            else
            {
                task->url.swap(task->redirectUrl);
                task->redirectUrl = "";
                LOG(debug) << "Resource '"
                           << resource->resource->name << "' redirected to '"
                           << task->url << "'";
                resources.fetcher->fetch(task);
                return;
            }
        }
    }
    
    resources.downloads--;
    
    if (resource->state == ResourceImpl::State::error)
    {
        resource->contentData.free();
        boost::lock_guard<boost::mutex> l(resources.mutInvalidUrls);
        resources.invalidUrlLocked.insert(resource->resource->name);
        return;
    }
    
    resource->saveToCache(this);
    resource->state = ResourceImpl::State::downloaded;
}

void MapImpl::dataRenderInitialize()
{}

void MapImpl::dataRenderFinalize()
{
    resources.prepareQueNoLock.clear();
    resources.resources.clear();
}

bool MapImpl::dataRenderTick()
{
    statistics.currentResourcePreparing = resources.prepareQueNoLock.size()
            + statistics.currentResourceDownloads;
    if (statistics.frameIndex % 2)
    { // sync download queue
        {
            boost::lock_guard<boost::mutex> l(resources.mutPrepareQue);
            std::swap(resources.prepareQueNoLock, resources.prepareQueLocked);
        }
        resources.prepareQueNoLock.clear();
    }
    else
    { // clear old resources
        std::vector<Resource*> res;
        res.reserve(resources.resources.size());
        uint64 memRamUse = 0;
        uint64 memGpuUse = 0;
        for (auto &&it : resources.resources)
        {
            memRamUse += it.second->ramMemoryCost;
            memGpuUse += it.second->gpuMemoryCost;
            // consider long time not used resources only
            if (it.second->impl->lastAccessTick + 100 < statistics.frameIndex
                    && it.second.use_count() == 1 && it.second->impl->state
                    != ResourceImpl::State::downloading)
                res.push_back(it.second.get());
        }
        uint64 memUse = memRamUse + memGpuUse;
        if (memUse > options.maxResourcesMemory)
        {
            std::sort(res.begin(), res.end(), [](Resource *a, Resource *b){
                if (a->impl->lastAccessTick == b->impl->lastAccessTick)
                    return a->gpuMemoryCost + a->ramMemoryCost
                            > b->gpuMemoryCost + b->ramMemoryCost;
                return a->impl->lastAccessTick < b->impl->lastAccessTick;
            });
            for (Resource *it : res)
            {
                if (memUse <= options.maxResourcesMemory)
                    break;
                memUse -= it->gpuMemoryCost + it->ramMemoryCost;
                if (it->impl->state != ResourceImpl::State::finalizing)
                    it->impl->state = ResourceImpl::State::finalizing;
                else
                {
                    statistics.resourcesReleased++;
                    LOG(info2) << "Releasing resource '" << it->name << "'";
                    resources.resources.erase(it->name);
                }
            }
        }
        statistics.currentGpuMemUse = memGpuUse;
        statistics.currentRamMemUse = memRamUse;
        statistics.currentResources = resources.resources.size();
    }
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
        resources.prepareQueNoLock.insert(resource->impl.get());
        break;
    }
}

std::shared_ptr<GpuTexture> MapImpl::getTexture(const std::string &name)
{
    auto it = resources.resources.find(name);
    if (it == resources.resources.end())
    {
        resources.resources[name] = mapFoundation->createTexture(name);
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
        resources.resources[name] = mapFoundation->createMesh(name);
        it = resources.resources.find(name);
    }
    touchResource(it->second);
    return std::dynamic_pointer_cast<GpuMesh>(it->second);
}

std::shared_ptr<MapConfig> MapImpl::getMapConfig(const std::string &name)
{
    return getMapResource<MapConfig>(name, this,
                    std::numeric_limits<double>::infinity());
}

std::shared_ptr<MetaTile> MapImpl::getMetaTile(const std::string &name)
{
    return getMapResource<MetaTile>(name, this, 1e5);
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
		assert(false);
    }
}

const std::string MapImpl::convertNameToCache(const std::string &path)
{
    uint32 p = path.find("://");
    std::string a = p == std::string::npos ? path : path.substr(p + 3);
    std::string b = boost::filesystem::path(a).parent_path().string();
    std::string c = a.substr(b.length() + 1);
    if (b.empty() || c.empty())
        LOGTHROW(err2, std::runtime_error)
                << "Cannot convert path '" << path
                << "' into a cache path";
    return resources.cachePath
            + convertNameToPath(b, false) + "/"
            + convertNameToPath(c, false);
}

bool MapImpl::availableInCache(const std::string &name)
{
    std::string path = convertNameToCache(name);
    return boost::filesystem::exists(path);
}

const std::string MapImpl::convertNameToPath(std::string path,
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

const std::string MapImpl::Resources::invalidUrlFileName = "invalidUrl.txt";

} // namespace vts
