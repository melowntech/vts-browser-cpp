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

#include "include/vts-browser/map.hpp"
#include "map.hpp"

namespace vts
{

namespace
{

template<class T>
std::shared_ptr<T> getMapResource(MapImpl *map, const std::string &name)
{
    auto it = map->resources.resources.find(name);
    if (it == map->resources.resources.end())
    {
        auto r = std::make_shared<T>(map, name);
        map->resources.resources[name] = r;
        it = map->resources.resources.find(name);
    }
    map->touchResource(it->second);
    return std::dynamic_pointer_cast<T>(it->second);
}

void initializeFetchTask(MapImpl *map, Resource *task)
{
    task->queryHeaders["X-Vts-Client-Id"] = map->clientId;
    if (map->auth)
        map->auth->authorize(task);
}

bool allowDiskCache(Resource::ResourceType type)
{
    switch (type)
    {
    case Resource::ResourceType::AuthConfig:
    case Resource::ResourceType::MapConfig:
    case Resource::ResourceType::BoundLayerConfig:
        return false;
    default:
        return true;
    }
}

} // namespace

MapImpl::Resources::Resources()
    : downloads(0), fetcher(nullptr)
{}

FetchTask::FetchTask(const std::string &url, ResourceType resourceType) :
    resourceType(resourceType), queryUrl(url), replyExpires(-1), replyCode(0)
{}

FetchTask::~FetchTask()
{}

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
    initializeFetchTask(map, this);
}

Resource::~Resource()
{}

Resource::operator bool() const
{
    return state == Resource::State::ready;
}

////////////////////////////
/// DATA THREAD
////////////////////////////

void MapImpl::resourceLoad(const std::shared_ptr<Resource> &r)
{
    assert(r->state == Resource::State::initializing);
    r->contentData.free();
    r->replyCode = 0;
    r->replyExpires = -1;
    try
    {
        if (allowDiskCache(r->resourceType)
                && cache->read(r->name, r->contentData, r->replyExpires))
        {
            if (r->contentData.size() > 0)
            {
                statistics.resourcesDiskLoaded++;
                r->state = Resource::State::downloaded;
            }
            else
            {
                statistics.resourcesIgnored++;
                r->state = Resource::State::availFail;
            }
            r->replyCode = 200;
        }
        else if (r->name.find("://") == std::string::npos)
        {
            r->contentData = readInternalMemoryBuffer(r->name);
            r->state = Resource::State::downloaded;
            r->replyCode = 200;
        }
        else if (resources.downloads < options.maxConcurrentDownloads)
        {
            statistics.resourcesDownloaded++;
            resources.fetcher->fetch(r);
            resources.downloads++;
            r->state = Resource::State::downloading;
        }
    }
    catch (const std::exception &e)
    {
        statistics.resourcesFailed++;
        r->state = Resource::State::errorFatal;
        LOG(err3) << "Failed processing resource <"
                  << r->name
                  << ">, exception: " << e.what();
    }
}

void Resource::processLoad()
{
    info.gpuMemoryCost = info.ramMemoryCost = 0;
    map->statistics.resourcesProcessLoaded++;
    try
    {
        load();
    }
    catch (const std::exception &e)
    {
        if (map->options.debugSaveCorruptedFiles)
        {
            try
            {
                writeLocalFileBuffer(std::string() + "corrupted/"
                    + convertNameToPath(name, false), contentData);
            }
            catch(...)
            {
                // do nothing
            }
        }
        map->statistics.resourcesFailed++;
		contentData.free();
        state = Resource::State::errorFatal;
        LOG(err3) << "Failed processing resource <" << name
                  << ">, exception: " << e.what();
        return;
    }
    contentData.free();
    state = Resource::State::ready;
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
    statistics.currentResourceDownloads = resources.downloads;

    // sync resources
    std::vector<std::shared_ptr<Resource>> res;
    {
        boost::lock_guard<boost::mutex> l(resources.mutResourcesCopy);
        if (resources.resourcesCopy.empty())
            return true; // all done
        res.swap(resources.resourcesCopy);
    }

    // sort resources by priority
    for (auto &&it : res)
    {
#ifndef NDEBUG
        if (!(it->priority == it->priority))
        {
            LOG(warn3) << "resource <"
                       << it->name
                       << "> does not have assigned priority";
        }
#endif

        it->priorityCopy = it->priority;
        it->priority *= 0.1;
    }
    std::sort(res.begin(), res.end(), [](
              const std::shared_ptr<Resource> &a,
              const std::shared_ptr<Resource> &b
        ){ return a->priorityCopy > b->priorityCopy; });

    // process the resources
    uint32 processed = 0;
    for (std::shared_ptr<Resource> r : res)
    {
        if (processed++ >= options.maxResourceProcessesPerTick)
            return false; // tasks left
        switch (r->state)
        {
        case Resource::State::downloaded:
            r->processLoad();
            break;
        case Resource::State::initializing:
            resourceLoad(r);
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
        if (availTest->codes.find(replyCode) == availTest->codes.end())
            return false;
        break;
    case vtslibs::registry::BoundLayer::Availability::Type::negativeType:
        if (availTest->mime == contentType)
            return false;
        break;
    case vtslibs::registry::BoundLayer::Availability::Type::negativeSize:
        if (contentData.size() <= (unsigned)availTest->size)
            return false;
        break;
    }
    return true;
}

void Resource::fetchDone()
{
    LOG(debug) << "Fetched file <" << name << ">";
    assert(map);
    map->resources.downloads--;
    state = Resource::State::downloading;

    // handle error or invalid codes
    if (replyCode >= 400 || replyCode < 200)
    {
        LOG(err2) << "Error downloading <" << name
                  << ">, http code " << replyCode;
        state = Resource::State::errorRetry;
    }

    // these resources must always revalidate
    if (!allowDiskCache(resourceType))
        replyExpires = -2;

    // availability tests
    if (state == Resource::State::downloading
            && !performAvailTest())
    {
        LOG(info1) << "Availability test failed for resource <"
                   << name << ">";
        state = Resource::State::availFail;
        contentData.free();
        map->cache->write(name, contentData, replyExpires);
    }

    // handle redirections
    if (state == Resource::State::downloading
            && replyCode >= 300 && replyCode < 400)
    {
        if (redirectionsCount++ > map->options.maxFetchRedirections)
        {
            LOG(err2) << "Too many redirections in <"
                      << name << ">, last url <"
                      << queryUrl << ">, http code " << replyCode;
            state = Resource::State::errorRetry;
        }
        else
        {
            queryUrl.swap(replyRedirectUrl);
            replyRedirectUrl = "";
            LOG(info1) << "Download of <"
                       << name << "> redirected to <"
                       << queryUrl << ">, http code " << replyCode;
            replyCode = 0;
            state = Resource::State::initializing;
            return;
        }
    }

    if (state != Resource::State::downloading)
    {
        contentData.free();
        return;
    }

    map->cache->write(name, contentData, replyExpires);
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

void MapImpl::resourceRenderInitialize()
{}

void MapImpl::resourceRenderFinalize()
{
    resources.resources.clear();
}

void MapImpl::resourceRenderTick()
{
    // clear old resources
    if (statistics.frameIndex % 30 == 0)
    {
        std::vector<std::shared_ptr<Resource>> res;
        res.reserve(resources.resources.size());
        uint64 memRamUse = 0;
        uint64 memGpuUse = 0;
        for (auto &&it : resources.resources)
        {
            memRamUse += it.second->info.ramMemoryCost;
            memGpuUse += it.second->info.gpuMemoryCost;
            // consider long time not used resources only
            if (it.second->lastAccessTick + 100 < statistics.frameIndex)
                res.push_back(it.second);
        }
        uint64 memUse = memRamUse + memGpuUse;
        if (memUse > options.maxResourcesMemory)
        {
            std::sort(res.begin(), res.end(), [](std::shared_ptr<Resource> &a,
                      std::shared_ptr<Resource> &b){
                return a->lastAccessTick < b->lastAccessTick;
            });
            for (auto &&it : res)
            {
                if (memUse <= options.maxResourcesMemory)
                    break;
                memUse -= it->info.gpuMemoryCost + it->info.ramMemoryCost;
                if (it->state != Resource::State::finalizing)
                    it->state = Resource::State::finalizing;
                else
                {
                    statistics.resourcesReleased++;
                    LOG(info1) << "Releasing resource <"
                               << it->name << ">";
                    resources.resources.erase(it->name);
                }
            }
        }
        statistics.currentGpuMemUse = memGpuUse;
        statistics.currentRamMemUse = memRamUse;
    }
    statistics.currentResources = resources.resources.size();

    // check which resources need attention
    {
        std::vector<std::shared_ptr<Resource>> res;
        res.reserve(resources.resources.size());
        std::time_t current = std::time(nullptr);
        for (auto it : resources.resources)
        {
            std::shared_ptr<Resource> &r = it.second;
            if (r->lastAccessTick != statistics.frameIndex)
                continue;
            switch (r->state)
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
                    r->retryTime = (1 << r->retryNumber) * 15 + current;
                    LOG(warn2) << "Resource <" << r->name
                               << "> will retry in "
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
                // no break here
            case Resource::State::finalizing:
                r->state = Resource::State::initializing;
            case Resource::State::initializing:
            case Resource::State::downloaded:
                res.push_back(r);
                break;
            case Resource::State::ready:
                if (options.enableRuntimeResourceExpiration
                        && r->replyExpires > 0 && r->replyExpires < current)
                {
                    LOG(info1) << "Resource <" << r->name
                               << "> has expired (expiration: "
                               << r->replyExpires << ", current: "
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
        // sync resources copy
        {
            boost::lock_guard<boost::mutex> l(resources.mutResourcesCopy);
            res.swap(resources.resourcesCopy);
        }
        statistics.currentResourcePreparing = res.size() + resources.downloads;
    }
}

void MapImpl::touchResource(const std::shared_ptr<Resource> &resource)
{
    resource->lastAccessTick = statistics.frameIndex;
}

std::shared_ptr<GpuTexture> MapImpl::getTexture(const std::string &name)
{
    return getMapResource<GpuTexture>(this, name);
}

std::shared_ptr<GpuMesh> MapImpl::getMeshRenderable(const std::string &name)
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

std::shared_ptr<BoundMetaTile> MapImpl::getBoundMetaTile(
        const std::string &name)
{
    return getMapResource<BoundMetaTile>(this, name);
}

std::shared_ptr<BoundMaskTile> MapImpl::getBoundMaskTile(
        const std::string &name)
{
    return getMapResource<BoundMaskTile>(this, name);
}

std::shared_ptr<SearchTaskImpl> MapImpl::getSearchTask(const std::string &name)
{
    return getMapResource<SearchTaskImpl>(this, name);
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
    switch (resource->state)
    {
    case Resource::State::errorFatal:
        return Validity::Invalid;
    case Resource::State::ready:
        return Validity::Valid;
    default:
        return Validity::Indeterminate;
    }
}

} // namespace vts
