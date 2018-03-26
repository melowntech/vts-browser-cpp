/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "../include/vts-browser/map.hpp"
#include "../map.hpp"

namespace vts
{

namespace
{

template<class T>
std::shared_ptr<T> getMapResource(MapImpl *map, const std::string &name)
{
    assert(!name.empty());
    auto it = map->resources.resources.find(name);
    if (it == map->resources.resources.end())
    {
        auto r = std::make_shared<T>(map, name);
        map->resources.resources[name] = r;
        it = map->resources.resources.find(name);
        map->statistics.resourcesCreated++;
    }
    assert(it->second);
    map->touchResource(it->second);
    auto res = std::dynamic_pointer_cast<T>(it->second);
    assert(res);
    return res;
}

void initializeFetchTask(MapImpl *map, const std::shared_ptr<Resource> &task)
{
    LOG(debug) << "Resource <" << task->name << "> initializing fetch";
    task->query.headers["X-Vts-Client-Id"] = map->createOptions.clientId;
    if (map->resources.auth)
        map->resources.auth->authorize(task);
    if (task->query.url.empty())
        task->query.url = task->name;
}

bool allowDiskCache(Resource::ResourceType type)
{
    switch (type)
    {
    case Resource::ResourceType::AuthConfig:
    case Resource::ResourceType::MapConfig:
    case Resource::ResourceType::BoundLayerConfig:
    case Resource::ResourceType::TilesetMappingConfig:
        return false;
    default:
        return true;
    }
}

bool startsWith(const std::string &text, const std::string &start)
{
    return text.substr(0, start.length()) == start;
}

void releaseIfLast(std::shared_ptr<Resource> &p)
{
    std::weak_ptr<Resource> w = p;
    p.reset();
    p = w.lock();
}

} // namespace

MapImpl::Resources::Resources() : downloads(0), tickIndex(0),
    progressEstimationMaxResources(0)
{}

bool FetchTask::isResourceTypeMandatory(FetchTask::ResourceType resourceType)
{
    return !allowDiskCache(resourceType);
}

uint32 gpuTypeSize(GpuTypeEnum type)
{
    switch (type)
    {
    case GpuTypeEnum::Byte:
    case GpuTypeEnum::UnsignedByte:
        return 1;
    case GpuTypeEnum::Short:
    case GpuTypeEnum::UnsignedShort:
        return 2;
    case GpuTypeEnum::Int:
    case GpuTypeEnum::UnsignedInt:
    case GpuTypeEnum::Float:
        return 4;
    default:
        LOGTHROW(fatal, std::invalid_argument) << "invalid gpu type enum";
        throw;
    }
}

ResourceInfo::ResourceInfo() :
    ramMemoryCost(0), gpuMemoryCost(0)
{}

Resource::Resource(vts::MapImpl *map, const std::string &name,
                   FetchTask::ResourceType resourceType) : 
    FetchTask(name, resourceType), name(name), map(map),
    state(State::initializing), retryTime(-1), retryNumber(0),
    redirectionsCount(0), lastAccessTick(0),
    priority(std::numeric_limits<float>::quiet_NaN()),
    priorityCopy(std::numeric_limits<float>::quiet_NaN())
{
    LOG(debug) << "Constructing resource <" << name
               << "> at <" << this << ">";
}

Resource::~Resource()
{
    LOG(debug) << "Destroying resource <" << name
               << "> at <" << this << ">";
}

Resource::operator bool() const
{
    return state == Resource::State::ready;
}

std::ostream &operator << (std::ostream &stream, Resource::State state)
{
    switch (state)
    {
        case Resource::State::initializing:
            stream << "initializing";
            break;
        case Resource::State::downloading:
            stream << "downloading";
            break;
        case Resource::State::downloaded:
            stream << "downloaded";
            break;
        case Resource::State::ready:
            stream << "ready";
            break;
        case Resource::State::errorFatal:
            stream << "errorFatal";
            break;
        case Resource::State::errorRetry:
            stream << "errorRetry";
            break;
        case Resource::State::availFail:
            stream << "availFail";
            break;
    }
    return stream;
}

////////////////////////////
/// DATA THREAD
////////////////////////////

void MapImpl::resourceInitializeDownload(const std::shared_ptr<Resource> &r)
{
    assert(r->state == Resource::State::initializing);
    r->reply.content.free();
    std::string().swap(r->reply.contentType);
    r->reply.code = 0;
    r->reply.expires = -1;
    r->info.gpuMemoryCost = r->info.ramMemoryCost = 0;
    try
    {
        if (allowDiskCache(r->query.resourceType) && resources.cache->read(
                    r->name, r->reply.content, r->reply.expires))
        {
            statistics.resourcesDiskLoaded++;
            if (r->reply.content.size() > 0)
                r->state = Resource::State::downloaded;
            else
                r->state = Resource::State::availFail;
            r->reply.code = 200;
        }
        else if (startsWith(r->name, "file://"))
        {
            r->reply.content = readLocalFileBuffer(r->name.substr(7));
            r->state = Resource::State::downloaded;
            r->reply.code = 200;
        }
        else if (startsWith(r->name, "internal://"))
        {
            r->reply.content = readInternalMemoryBuffer(r->name.substr(11));
            r->state = Resource::State::downloaded;
            r->reply.code = 200;
        }
        else if (resources.downloads < options.maxConcurrentDownloads)
        {
            statistics.resourcesDownloaded++;
            resources.downloads++;
            r->state = Resource::State::downloading;
            initializeFetchTask(this, r);
            resources.fetcher->fetch(r);
        }
    }
    catch (const std::exception &e)
    {
        statistics.resourcesFailed++;
        r->state = Resource::State::errorFatal;
        LOG(err3) << "Failed loading resource <" << r->name
                  << ">, exception <" << e.what() << ">";
    }
}

void Resource::processLoad()
{
    assert(state == Resource::State::downloaded);
    info.gpuMemoryCost = info.ramMemoryCost = 0;
    map->statistics.resourcesProcessed++;
    try
    {
        load();
        state = Resource::State::ready;
    }
    catch (const std::exception &e)
    {
        LOG(err3) << "Failed processing resource <" << name
                  << ">, exception <" << e.what() << ">";
        if (map->options.debugSaveCorruptedFiles)
        {
            std::string path = std::string() + "corrupted/"
                    + convertNameToPath(name, false);
            try
            {
                writeLocalFileBuffer(path, reply.content);
                LOG(info1) << "Resource <" << name
                           << "> saved into file <"
                           << path << "> for further inspection";
            }
            catch(...)
            {
                LOG(warn1) << "Failed saving resource <" << name
                           << "> into file <"
                           << path << "> for further inspection";
            }
        }
        map->statistics.resourcesFailed++;
        state = Resource::State::errorFatal;
    }
    reply.content.free();
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
    {
        boost::lock_guard<boost::mutex> l(resources.mutResourcesCopy);
        resources.resourcesCopy.clear();
    }
}

bool MapImpl::resourceDataTick()
{
    statistics.resourcesDownloading = resources.downloads;

    // sync resources
    std::vector<std::shared_ptr<Resource>> res;
    {
        boost::lock_guard<boost::mutex> l(resources.mutResourcesCopy);
        if (resources.resourcesCopy.empty())
            return true; // all done
        res.swap(resources.resourcesCopy);
    }

    // sort resources by priority
    for (auto &it : res)
    {
        assert(it->priority == it->priority);
        it->priorityCopy = it->priority;
        if (it->priority < std::numeric_limits<float>::infinity())
            it->priority = 0;
    }
    std::sort(res.begin(), res.end(), [](
              const std::shared_ptr<Resource> &a,
              const std::shared_ptr<Resource> &b
        ){ return a->priorityCopy > b->priorityCopy; });

    // process the resources
    uint32 processed = 0;
    for (const std::shared_ptr<Resource> &r : res)
    {
        if (processed++ >= options.maxResourceProcessesPerTick)
            return false; // tasks left
        switch ((Resource::State)r->state)
        {
        case Resource::State::downloaded:
            r->processLoad();
            break;
        case Resource::State::initializing:
            resourceInitializeDownload(r);
            break;
        default:
            break; // the resource state may have changed in another thread
        }
    }

    return true; // all done
}

////////////////////////////
/// DOWNLOAD THREAD
////////////////////////////

bool Resource::performAvailTest() const
{
    if (!availTest)
        return true;
    switch (availTest->type)
    {
    case vtslibs::registry::BoundLayer::Availability::Type::negativeCode:
        if (availTest->codes.find(reply.code) == availTest->codes.end())
            return false;
        break;
    case vtslibs::registry::BoundLayer::Availability::Type::negativeType:
        if (availTest->mime == reply.contentType)
            return false;
        break;
    case vtslibs::registry::BoundLayer::Availability::Type::negativeSize:
        if (reply.content.size() <= (unsigned)availTest->size)
            return false;
        break;
    }
    return true;
}

void Resource::fetchDone()
{
    LOG(debug) << "Resource <" << name << "> finished fetching";
    assert(map);
    assert(state == Resource::State::downloading);
    assert(info.ramMemoryCost == 0);
    assert(info.gpuMemoryCost == 0);
    map->resources.downloads--;

    // handle error or invalid codes
    if (reply.code >= 400 || reply.code < 200)
    {
        if (reply.code == FetchTask::ExtraCodes::ProhibitedContent)
        {
            state = Resource::State::errorFatal;
        }
        else
        {
            LOG(err2) << "Error downloading <" << name
                      << ">, http code " << reply.code;
            state = Resource::State::errorRetry;
        }
    }

    // some resources must always revalidate
    if (!allowDiskCache(query.resourceType))
        reply.expires = -2;

    // availability tests
    if (state == Resource::State::downloading
            && !performAvailTest())
    {
        LOG(info1) << "Resource <" << name
                   << "> failed availability test";
        state = Resource::State::availFail;
        reply.content.free();
        map->resources.cache->write(name, reply.content, reply.expires);
    }
    std::string().swap(reply.contentType);

    // handle redirections
    if (state == Resource::State::downloading
            && reply.code >= 300 && reply.code < 400)
    {
        if (redirectionsCount++ > map->options.maxFetchRedirections)
        {
            LOG(err2) << "Too many redirections in <"
                      << name << ">, last url <"
                      << query.url << ">, http code " << reply.code;
            state = Resource::State::errorRetry;
        }
        else
        {
            query.url.swap(reply.redirectUrl);
            std::string().swap(reply.redirectUrl);
            LOG(info1) << "Download of <"
                       << name << "> redirected to <"
                       << query.url << ">, http code " << reply.code;
            reply.code = 0;
            reply.content.free();
            state = Resource::State::initializing;
            return;
        }
    }

    if (state != Resource::State::downloading)
    {
        reply.content.free();
        return;
    }

    std::string().swap(query.url);
    query.headers.clear();
    map->resources.cache->write(name, reply.content, reply.expires);
    info.ramMemoryCost = reply.content.size();
    retryNumber = 0; // reset counter
    state = Resource::State::downloaded;
}

////////////////////////////
/// RENDER THREAD
////////////////////////////

void Resource::updatePriority(float p)
{
    if (priority == priority)
        priority = std::max(priority, p);
    else
        priority = p;
}

void Resource::reset()
{
    if (state == Resource::State::downloading)
        return;
    retryNumber = 0;
    state = Resource::State::errorRetry;
}

void MapImpl::resourceRenderInitialize()
{}

void MapImpl::resourceRenderFinalize()
{
    resources.resources.clear();
}

void MapImpl::resourceRenderTick()
{
    if (renderer.tickIndex % 4 == 0)
    {
        // clear old resources
        struct Res
        {
            std::string s;
            uint32 p;
            bool operator < (const Res &other) const
            {
                return p < other.p;
            }
            Res(const std::string &s, uint32 p) : s(s), p(p)
            {}
        };
        std::vector<Res> resToRemove;
        uint64 memRamUse = 0;
        uint64 memGpuUse = 0;
        for (auto &it : resources.resources)
        {
            memRamUse += it.second->info.ramMemoryCost;
            memGpuUse += it.second->info.gpuMemoryCost;
            // consider long time not used resources only
            if (it.second->lastAccessTick + 5 < renderer.tickIndex)
                resToRemove.emplace_back(it.first, it.second->lastAccessTick);
        }
        uint64 memUse = memRamUse + memGpuUse;
        if (memUse > options.targetResourcesMemory)
        {
            std::sort(resToRemove.begin(), resToRemove.end());
            for (Res &res : resToRemove)
            {
                auto &it = resources.resources[res.s];
                std::string name = it->name;
                uint64 mem = it->info.gpuMemoryCost + it->info.ramMemoryCost;
                releaseIfLast(it);
                if (!it)
                {
                    LOG(info1) << "Released resource <" << name << ">";
                    resources.resources.erase(name);
                    statistics.resourcesReleased++;
                    memUse -= mem;
                    if (memUse <= options.targetResourcesMemory)
                        break;
                }
            }
        }
        statistics.currentGpuMemUse = memGpuUse;
        statistics.currentRamMemUse = memRamUse;
    }

    if (renderer.tickIndex % 4 == 2)
    {
        // check which resources need attention
        std::vector<std::shared_ptr<Resource>> res;
        res.reserve(resources.resources.size());
        std::time_t current = std::time(nullptr);
        for (auto it : resources.resources)
        {
            const std::shared_ptr<Resource> &r = it.second;
            if (r->lastAccessTick + 1 != renderer.tickIndex)
                continue;
            switch ((Resource::State)r->state)
            {
            case Resource::State::errorRetry:
                if (r->retryNumber >= options.maxFetchRetries)
                {
                    LOG(err3) << "All retries for resource <" << r->name
                               << "> have failed";
                    r->state = Resource::State::errorFatal;
                    statistics.resourcesFailed++;
                    break;
                }
                if (r->retryTime == -1)
                {
                    r->retryTime = (1 << r->retryNumber)
                            * options.fetchFirstRetryTimeOffset + current;
                    LOG(warn2) << "Resource <" << r->name
                               << "> may retry in "
                               << (r->retryTime - current) << " seconds";
                    break;
                }
                if (r->retryTime > current)
                    break;
                r->retryNumber++;
                LOG(info2) << "Trying again to download resource <"
                           << r->name << "> (attempt "
                           << r->retryNumber << ")";
                r->retryTime = -1;
                r->state = Resource::State::initializing;
                // no break here
            case Resource::State::initializing:
            case Resource::State::downloaded:
                res.push_back(r);
                break;
            case Resource::State::ready:
                if (options.enableRuntimeResourceExpiration
                        && r->reply.expires > 0 && r->reply.expires < current)
                {
                    LOG(info2) << "Resource <" << r->name
                               << "> has expired (expiration: "
                               << r->reply.expires << ", current: "
                               << current << ")";
                    r->state = Resource::State::initializing;
                    res.push_back(r);
                }
                break;
            case Resource::State::downloading:
            case Resource::State::errorFatal:
            case Resource::State::availFail:
                break;
            }
        }
        statistics.resourcesPreparing = res.size() + resources.downloads;
        // sync resources copy
        {
            boost::lock_guard<boost::mutex> l(resources.mutResourcesCopy);
            res.swap(resources.resourcesCopy);
        }
    }

    statistics.resourcesActive = resources.resources.size();
}

void MapImpl::touchResource(const std::shared_ptr<Resource> &resource)
{
    resource->lastAccessTick = renderer.tickIndex;
}

std::shared_ptr<GpuTexture> MapImpl::getTexture(const std::string &name)
{
    return getMapResource<GpuTexture>(this, name);
}

std::shared_ptr<GpuMesh> MapImpl::getMesh(const std::string &name)
{
    return getMapResource<GpuMesh>(this, name);
}

std::shared_ptr<AuthConfig> MapImpl::getAuthConfig(const std::string &name)
{
    return getMapResource<AuthConfig>(this, name);
}

std::shared_ptr<MapConfig> MapImpl::getMapConfig(const std::string &name)
{
    return getMapResource<MapConfig>(this, name);
}

std::shared_ptr<MetaTile> MapImpl::getMetaTile(const std::string &name)
{
    return getMapResource<MetaTile>(this, name);
}

std::shared_ptr<NavTile> MapImpl::getNavTile(
        const std::string &name)
{
    return getMapResource<NavTile>(this, name);
}

std::shared_ptr<MeshAggregate> MapImpl::getMeshAggregate(
        const std::string &name)
{
    return getMapResource<MeshAggregate>(this, name);
}

std::shared_ptr<ExternalBoundLayer> MapImpl::getExternalBoundLayer(
        const std::string &name)
{
    return getMapResource<ExternalBoundLayer>(this, name);
}

std::shared_ptr<ExternalFreeLayer> MapImpl::getExternalFreeLayer(
        const std::string &name)
{
    return getMapResource<ExternalFreeLayer>(this, name);
}

std::shared_ptr<BoundMetaTile> MapImpl::getBoundMetaTile(
        const std::string &name)
{
    return getMapResource<BoundMetaTile>(this, name);
}

std::shared_ptr<SearchTaskImpl> MapImpl::getSearchTask(const std::string &name)
{
    return getMapResource<SearchTaskImpl>(this, name);
}

std::shared_ptr<TilesetMapping> MapImpl::getTilesetMapping(
        const std::string &name)
{
    return getMapResource<TilesetMapping>(this, name);
}

std::shared_ptr<SriIndex> MapImpl::getSriIndex(const std::string &name)
{
    return getMapResource<SriIndex>(this, name);
}

std::shared_ptr<GeodataFeatures> MapImpl::getGeoFeatures(
        const std::string &name)
{
    return getMapResource<GeodataFeatures>(this, name);
}

std::shared_ptr<GeodataStylesheet> MapImpl::getGeoStyle(
        const std::string &name)
{
    return getMapResource<GeodataStylesheet>(this, name);
}

std::shared_ptr<GpuGeodata> MapImpl::getGeodata(const std::string &name)
{
    return getMapResource<GpuGeodata>(this, name);
}

Validity MapImpl::getResourceValidity(const std::string &name)
{
    auto it = resources.resources.find(name);
    if (it == resources.resources.end())
        return Validity::Invalid;
    return getResourceValidity(it->second);
}

Validity MapImpl::getResourceValidity(const std::shared_ptr<Resource> &resource)
{
    switch ((Resource::State)resource->state)
    {
    case Resource::State::errorFatal:
    case Resource::State::availFail:
        return Validity::Invalid;
    case Resource::State::ready:
        return Validity::Valid;
    default:
        return Validity::Indeterminate;
    }
}

} // namespace vts
