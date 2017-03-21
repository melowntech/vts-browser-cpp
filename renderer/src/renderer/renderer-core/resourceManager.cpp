#include "map.h"

namespace melown
{

MapImpl::Resources::Resources() : takeItemIndex(0), tickIndex(0), downloads(0),
    destroyTheFetcher(false)
{
    try
    {
        if (boost::filesystem::exists(invalidUrlFileName))
        {
            std::ifstream f;
            f.open(invalidUrlFileName);
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
        f.open(invalidUrlFileName);
        for (auto &&line : invalidUrl)
            f << line << '\n';
        f.close();
    }
    catch (...)
    {}
}

void MapImpl::dataInitialize(GpuContext *context, Fetcher *fetcher)
{
    dataContext = context;
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
    dataContext = nullptr;
    resources.fetcher->finalize();
    resources.prepareQue.clear();
}

void MapImpl::loadResource(ResourceImpl *r)
{
    assert(r->download);
    statistics.resourcesGpuLoaded++;
    try
    {
        r->resource->load(this);
        r->state = ResourceImpl::State::ready;
    }
    catch (std::runtime_error &)
    {
        LOG(err3) << "Error loading resource: " + r->resource->name;
        statistics.resourcesFailed++;
        r->state = ResourceImpl::State::errorLoad;
    }
    r->download.reset();
}

bool MapImpl::dataTick()
{
    statistics.currentDownloads = resources.downloads;
    
    { // sync invalid urls
        boost::lock_guard<boost::mutex> l(resources.mutInvalidUrls);
        resources.invalidUrl.insert(resources.invalidUrlNew.begin(),
                                    resources.invalidUrlNew.end());
        resources.invalidUrlNew.clear();
    }
    
    ResourceImpl *r = nullptr;
    {
        boost::lock_guard<boost::mutex> l(resources.mutPrepareQue);
        if (!resources.prepareQue.empty())
        {
            auto it = std::next(resources.prepareQue.begin(),
                                resources.takeItemIndex++
                                % resources.prepareQue.size());
            r = *it;
            resources.prepareQue.erase(it);
        }
    }
    if (!r)
        return true; // sleep
    
    if (r->state == ResourceImpl::State::downloaded)
    {
        loadResource(r);
        return false;
    }
    
    if (r->state == ResourceImpl::State::initializing)
    {
        if (resources.invalidUrl.find(r->resource->name)
                != resources.invalidUrl.end())
        {
            statistics.resourcesIgnored++;
            r->state = ResourceImpl::State::errorLoad;
            return false;
        }
        
        if (r->resource->name.find("://") == std::string::npos)
        {
            assert(!r->download);
            r->download.emplace(r);
            r->download->readLocalFile();
            loadResource(r);
        }
        else if (r->resource->name.find(".json") == std::string::npos
                 && availableInCache(r->resource->name))
        {
            assert(!r->download);
            r->download.emplace(r);
            r->download->loadFromCache();
            loadResource(r);
            statistics.resourcesDiskLoaded++;
        }
        else if (resources.downloads < 5)
        {
            assert(!r->download);
            r->download.emplace(r);
            r->state = ResourceImpl::State::downloading;
            resources.fetcher->fetch(r->download.get_ptr());
            statistics.resourcesDownloaded++;
            resources.downloads++;
        }
        else
            return true; // sleep
        
        return false;
    }
    
    return true; // sleep
}

void MapImpl::fetchedFile(FetchTask *task)
{
    ResourceImpl *resource = ((ResourceImpl::DownloadTask*)task)->resource;
    assert(resource->state == ResourceImpl::State::downloading);
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
                if (task->contentData.size() <= resource->availTest->size)
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
                resources.fetcher->fetch(task);
                return;
            }
        }
    
    resources.downloads--;
    
    if (resource->state == ResourceImpl::State::errorDownload)
    {
        resource->download.reset();
        boost::lock_guard<boost::mutex> l(resources.mutInvalidUrls);
        resources.invalidUrlNew.insert(resource->resource->name);
        return;
    }
    
    resource->download->saveToCache();
    resource->state = ResourceImpl::State::downloaded;
}

void MapImpl::dataRenderInitialize()
{}

void MapImpl::dataRenderFinalize()
{
    renderContext = nullptr;
    resources.prepareQueNew.clear();
    LOG(info3) << "Releasing " << resources.resources.size() << " resources";
    resources.resources.clear();
}

bool MapImpl::dataRenderTick()
{
    { // sync download queue
        boost::lock_guard<boost::mutex> l(resources.mutPrepareQue);
        std::swap(resources.prepareQueNew, resources.prepareQue);
    }
    resources.prepareQueNew.clear();
    
    { // clear old resources
        std::vector<Resource*> res;
        res.reserve(resources.resources.size());
        uint32 memRamUse = 0;
        uint32 memGpuUse = 0;
        for (auto &&it : resources.resources)
        {
            memRamUse += it.second->ramMemoryCost;
            memGpuUse += it.second->gpuMemoryCost;
            // consider long time not used resources only
            if (it.second->impl->lastAccessTick + 100 < resources.tickIndex
                    && it.second->impl->state
                    != ResourceImpl::State::downloading)
                res.push_back(it.second.get());
        }
        uint32 memUse = memRamUse + memGpuUse;
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
                {
                    statistics.resourcesReleased++;
                    resources.resources.erase(it->name);
                }
            }
        }
        statistics.currentGpuMemUse = memGpuUse;
        statistics.currentRamMemUse = memRamUse;
    }
    
    // advance the tick counter
    resources.tickIndex++;
}

void MapImpl::touchResource(const std::string &, Resource *resource)
{
    resource->impl->lastAccessTick = resources.tickIndex;
    switch (resource->impl->state)
    {
    case ResourceImpl::State::finalizing:
        resource->impl->state = ResourceImpl::State::initializing;
        // intentionally no break here
    case ResourceImpl::State::initializing:
    case ResourceImpl::State::downloaded:
        resources.prepareQueNew.insert(resource->impl.get());
        break;
    }
}

GpuShader *MapImpl::getShader(const std::string &name)
{
    return getGpuResource<GpuShader, &GpuContext::createShader>(name);
}

GpuTexture *MapImpl::getTexture(const std::string &name)
{
    return getGpuResource<GpuTexture, &GpuContext::createTexture>(name);
}

GpuMeshRenderable *MapImpl::getMeshRenderable(const std::string &name)
{
    return getGpuResource<GpuMeshRenderable,
            &GpuContext::createMeshRenderable>(name);
}

MapConfig *MapImpl::getMapConfig(const std::string &name)
{
    return getMapResource<MapConfig>(name);
}

MetaTile *MapImpl::getMetaTile(const std::string &name)
{
    return getMapResource<MetaTile>(name);
}

NavTile *MapImpl::getNavTile(const std::string &name)
{
    return getMapResource<NavTile>(name);
}

MeshAggregate *MapImpl::getMeshAggregate(const std::string &name)
{
    return getMapResource<MeshAggregate>(name);
}

ExternalBoundLayer *MapImpl::getExternalBoundLayer(const std::string &name)
{
    return getMapResource<ExternalBoundLayer>(name);
}

BoundMetaTile *MapImpl::getBoundMetaTile(const std::string &name)
{
    return getMapResource<BoundMetaTile>(name);
}

BoundMaskTile *MapImpl::getBoundMaskTile(const std::string &name)
{
    return getMapResource<BoundMaskTile>(name);
}

bool MapImpl::getResourceReady(const std::string &name)
{
    auto &&it = resources.resources.find(name);
    if (it == resources.resources.end())
        return false;
    return *it->second;
}

const std::string MapImpl::Resources::invalidUrlFileName
            = "cache/invalidUrl.txt";

} // namespace melown
