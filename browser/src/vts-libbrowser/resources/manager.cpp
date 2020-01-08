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

#include <optick.h>

#include "../include/vts-browser/log.hpp"

#include "../fetchTask.hpp"
#include "../map.hpp"
#include "../authConfig.hpp"
#include "../utilities/dataUrl.hpp"

namespace vts
{

void MapImpl::resourceSaveCorruptedFile(const std::shared_ptr<Resource> &r)
{
    if (!options.debugSaveCorruptedFiles)
        return;
    try
    {
        std::string path = std::string() + "corrupted/"
            + convertNameToPath(r->name, false);
        writeLocalFileBuffer(path, r->fetch->reply.content);
        LOG(info1) << "Resource <" << r->name
            << "> saved into file <"
            << path << "> for further inspection";
    }
    catch (...)
    {
        LOG(warn1) << "Failed saving corrupted resource <" << r->name
            << "> for further inspection";
    }
}

////////////////////////////
// A FETCH THREAD
////////////////////////////

CacheData::CacheData() : expires(0), availFailed(false)
{}

CacheData::CacheData(FetchTaskImpl *task, bool availFailed) :
    //availTest(task->availTest),
    buffer(task->reply.content.copy()),
    name(task->name), expires(task->reply.expires),
    availFailed(availFailed)
{}

void FetchTaskImpl::fetchDone()
{
    OPTICK_EVENT();
    LOG(debug) << "Resource <" << name << "> finished downloading, "
        << "http code: " << reply.code << ", content type: <"
        << reply.contentType << ">, size: " << reply.content.size()
        << ", expires: " << reply.expires;
    assert(map);
    map->resources.downloads--;
    Resource::State state = Resource::State::downloading;

    // handle error or invalid codes
    if (reply.code >= 400 || reply.code < 200)
    {
        if (reply.code == FetchTask::ExtraCodes::ProhibitedContent)
            state = Resource::State::errorFatal;
        else
        {
            const auto r = resource.lock();
            LOGR(r && r->retryNumber < 2 ? dbglog::err1 : dbglog::err2)
                << "Error downloading <" << name
                << ">, http code " << reply.code;
            state = Resource::State::errorRetry;
        }
    }

    // some resources must always revalidate
    if (!Resource::allowDiskCache(query.resourceType))
        reply.expires = -2;

    // availability tests
    if (state == Resource::State::downloading
        && !performAvailTest())
    {
        LOG(info1) << "Resource <" << name
            << "> failed availability test";
        state = Resource::State::availFail;
    }

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
            LOG(info1) << "Download of <"
                << name << "> redirected to <"
                << query.url << ">, http code " << reply.code;
            reply = Reply();
            state = Resource::State::initializing;
        }
    }

    // write to cache
    if (state == Resource::State::availFail
        || state == Resource::State::downloading)
    {
        // this makes copy of the content buffer
        map->resources.queCacheWrite.push(CacheData(this,
            state == Resource::State::availFail));
    }

    // update the actual resource
    if (state == Resource::State::downloading)
        state = Resource::State::downloaded;
    else
    {
        reply.content.free();
        reply.code = 0;
    }
    {
        std::shared_ptr<Resource> rs = resource.lock();
        if (rs)
        {
            assert(&*rs->fetch == this);
            assert(rs->state == Resource::State::downloading);
            rs->info.ramMemoryCost = reply.content.size();
            rs->state = state;
            if (state == Resource::State::downloaded)
            {
                // this allows another thread to immediately start
                //   processing the content, including its modification,
                //   and must therefore be the last action in this thread
                map->resources.queDecode.push(rs);
            }
        }
    }
}

////////////////////////////
// DECODE THREAD
////////////////////////////

void MapImpl::resourceDecodeProcess(const std::shared_ptr<Resource> &r)
{
    OPTICK_EVENT();
    OPTICK_TAG("name", r->name.c_str());

    assert(r->state == Resource::State::downloaded);
    statistics.resourcesDecoded++;
    try
    {
        r->decode();
        r->state = Resource::State::decoded;
        resources.queUpload.push(r);
    }
    catch (const std::exception &e)
    {
        LOG(err3) << "Failed decoding resource <" << r->name
            << ">, exception <" << e.what() << ">";
        resourceSaveCorruptedFile(r);
        statistics.resourcesFailed++;
        r->state = Resource::State::errorFatal;
    }
    r->fetch.reset();
}

void MapImpl::resourcesDecodeProcessorEntry()
{
    OPTICK_THREAD("decode");
    setLogThreadName("decode");
    while (!resources.queDecode.stopped())
    {
        std::weak_ptr<Resource> w;
        resources.queDecode.waitPop(w);
        std::shared_ptr<Resource> r = w.lock();
        if (!r)
            continue;
        resourceDecodeProcess(r);
    }
}

////////////////////////////
// DATA THREAD
////////////////////////////

void MapImpl::resourceUploadProcess(const std::shared_ptr<Resource> &r)
{
    OPTICK_EVENT();
    OPTICK_TAG("name", r->name.c_str());

    assert(r->state == Resource::State::decoded);
    r->info.gpuMemoryCost = r->info.ramMemoryCost = 0;
    statistics.resourcesUploaded++;
    try
    {
        r->upload();
        r->state = Resource::State::ready;
    }
    catch (const std::exception &e)
    {
        LOG(err3) << "Failed uploading resource <" << r->name
            << ">, exception <" << e.what() << ">";
        resourceSaveCorruptedFile(r);
        statistics.resourcesFailed++;
        r->state = Resource::State::errorFatal;
    }
    r->decodeData.reset();
}

void MapImpl::resourceDataUpdate()
{
    OPTICK_EVENT();
    for (uint32 proc = 0; proc
        < options.maxResourceProcessesPerTick; proc++)
    {
        std::weak_ptr<Resource> w;
        if (!resources.queUpload.tryPop(w))
            break;
        std::shared_ptr<Resource> r = w.lock();
        if (!r)
            continue;
        resourceUploadProcess(r);
    }
}

void MapImpl::resourceDataRun()
{
    OPTICK_THREAD("data");
    while (!resources.queUpload.stopped())
    {
        std::weak_ptr<Resource> w;
        resources.queUpload.waitPop(w);
        std::shared_ptr<Resource> r = w.lock();
        if (!r)
            continue;
        resourceUploadProcess(r);
    }
}

////////////////////////////
// CACHE WRITE THREAD
////////////////////////////

void MapImpl::cacheWriteEntry()
{
    OPTICK_THREAD("cacheWriter");
    setLogThreadName("cache writer");
    while (!resources.queCacheWrite.stopped())
    {
        CacheData cwd;
        resources.queCacheWrite.waitPop(cwd);
        if (!cwd.name.empty())
            cacheWrite(std::move(cwd));
    }
}

////////////////////////////
// CACHE READ THREAD
////////////////////////////

void MapImpl::cacheReadEntry()
{
    OPTICK_THREAD("cacheReader");
    setLogThreadName("cache reader");
    while (!resources.queCacheRead.stopped())
    {
        std::weak_ptr<Resource> w;
        resources.queCacheRead.waitPop(w);
        std::shared_ptr<Resource> r = w.lock();
        if (!r)
            continue;
        try
        {
            cacheReadProcess(r);
        }
        catch (const std::exception &e)
        {
            statistics.resourcesFailed++;
            r->state = Resource::State::errorFatal;
            LOG(err3) << "Failed preparing resource <" << r->name
                << ">, exception <" << e.what() << ">";
        }
    }
}

namespace
{

bool startsWith(const std::string &text, const std::string &start)
{
    return text.substr(0, start.length()) == start;
}

} // namespace

void MapImpl::cacheReadProcess(const std::shared_ptr<Resource> &r)
{
    OPTICK_EVENT();
    assert(r->state == Resource::State::checkCache);
    if (!r->fetch)
        r->fetch = std::make_shared<FetchTaskImpl>(r);
    r->info.gpuMemoryCost = r->info.ramMemoryCost = 0;
    CacheData cd;
    if (r->allowDiskCache() && (cd = cacheRead(
        r->name)).name == r->name)
    {
        r->fetch->reply.expires = cd.expires;
        r->fetch->reply.content = std::move(cd.buffer);
        r->fetch->reply.code = 200;
        if (cd.availFailed)
            r->state = Resource::State::availFail;
        else
            r->state = Resource::State::downloaded;
        statistics.resourcesDiskLoaded++;
    }
    else if (startsWith(r->name, "data:"))
    {
        readDataUrl(r->name, r->fetch->reply.content,
            r->fetch->reply.contentType);
        r->fetch->reply.code = 200;
        r->state = Resource::State::downloaded;
    }
    else if (startsWith(r->name, "file://"))
    {
        r->fetch->reply.content = readLocalFileBuffer(r->name.substr(7));
        r->fetch->reply.code = 200;
        r->state = Resource::State::downloaded;
    }
    else if (startsWith(r->name, "internal://"))
    {
        r->fetch->reply.content = readInternalMemoryBuffer(r->name.substr(11));
        r->fetch->reply.code = 200;
        r->state = Resource::State::downloaded;
    }
    else if (startsWith(r->name, "generate://"))
    {
        r->state = Resource::State::errorRetry;
        // will be handled elsewhere
    }
    else
    {
        r->state = Resource::State::startDownload;
        // will be handled in main thread
    }

    if (r->state == Resource::State::downloaded)
        resources.queDecode.push(r);
}

////////////////////////////
// FETCHER THREAD
////////////////////////////

void MapImpl::resourcesDownloadsEntry()
{
    OPTICK_THREAD("fetcher");
    resources.fetcher->initialize();
    while (!resources.fetching.stop)
    {
        std::vector<std::weak_ptr<Resource>> res1;
        {
            std::unique_lock<std::mutex> lock(resources.fetching.mut);
            resources.fetching.con.wait(lock);
            res1.swap(resources.fetching.resources);
        }
        OPTICK_EVENT("process");
        resources.fetcher->update();
        if (resources.downloads >= options.maxConcurrentDownloads)
            continue; // skip processing if no download slots are available
        typedef std::pair<float, std::shared_ptr<Resource>> PR;
        std::vector<PR> res;
        for (const auto &w : res1)
        {
            std::shared_ptr<Resource> r = w.lock();
            if (r)
            {
                res.emplace_back(r->priority, r);
                if (r->priority < std::numeric_limits<float>::infinity())
                    r->priority = 0;
            }
        }
        std::sort(res.begin(), res.end(), [](PR &a, PR &b) {
            return a.first > b.first; // highest priority first
            });
        for (const auto &pr : res)
        {
            if (resources.downloads >= options.maxConcurrentDownloads)
                break;
            const std::shared_ptr<Resource> &r = pr.second;
            r->state = Resource::State::downloading;
            resources.downloads++;
            LOG(debug) << "Initializing fetch of <" << r->name << ">";
            r->fetch->query.headers["X-Vts-Client-Id"]
                = createOptions.clientId;
            if (resources.auth)
                resources.auth->authorize(r);
            resources.fetcher->fetch(r->fetch);
            statistics.resourcesDownloaded++;
        }
    }
    resources.fetcher->finalize();
    resources.fetcher.reset();
}

////////////////////////////
// MAIN THREAD
////////////////////////////

bool MapImpl::resourcesTryRemove(std::shared_ptr<Resource> &r)
{
    std::string name = r->name;
    assert(resources.resources.count(name) == 1);
    {
        // release the pointer if we are the last one holding it
        std::weak_ptr<Resource> w = r;
        try
        {
            r.reset();
        }
        catch (...)
        {
            LOGTHROW(fatal, std::logic_error)
                    << "Exception in destructor";
        }
        r = w.lock();
    }
    if (!r)
    {
        LOG(info1) << "Released resource <" << name << ">";
        resources.resources.erase(name);
        statistics.resourcesReleased++;
        return true;
    }
    return false;
}

void MapImpl::resourcesRemoveOld()
{
    OPTICK_EVENT();
    struct Res
    {
        std::string n; // name
        uint32 m; // memory used
        uint32 a; // lastAccessTick
        Res(const std::string &n, uint32 m, uint32 a) : n(n), m(m), a(a)
        {}
    };
    // successfully loaded resources are removed
    //   only when we are tight on memory
    std::vector<Res> resourcesToRemove;
    resourcesToRemove.reserve(resources.resources.size() / 4);
    // resources that errored are removed immediately
    std::vector<Res> unconditionalToRemove;
    unconditionalToRemove.reserve(resources.resources.size() / 4);
    uint64 memRamUse = 0;
    uint64 memGpuUse = 0;
    // find resource candidates for removal
    for (const auto &it : resources.resources)
    {
        resourceUpdateStatistics(it.second);
        memRamUse += it.second->info.ramMemoryCost;
        memGpuUse += it.second->info.gpuMemoryCost;
        // skip recently used resources
        if (it.second->lastAccessTick + 5 < renderTickIndex)
        {
            Res r(it.first,
                it.second->info.ramMemoryCost + it.second->info.gpuMemoryCost,
                it.second->lastAccessTick);
            switch ((vts::Resource::State)it.second->state)
            {
            case Resource::State::initializing:
            case Resource::State::startDownload:
            case Resource::State::errorFatal:
            case Resource::State::errorRetry:
            case Resource::State::availFail:
                unconditionalToRemove.push_back(std::move(r));
                break;
            default:
                resourcesToRemove.push_back(std::move(r));
                break;
            }
        }
    }
    statistics.currentGpuMemUseKB = memGpuUse / 1024;
    statistics.currentRamMemUseKB = memRamUse / 1024;
    uint64 memUse = memRamUse + memGpuUse;
    // remove unconditionalToRemove
    for (const Res &res : unconditionalToRemove)
    {
        if (resourcesTryRemove(resources.resources[res.n]))
            memUse -= res.m;
    }
    // remove resourcesToRemove
    uint64 trs = (uint64)options.targetResourcesMemoryKB * 1024;
    if (memUse > trs)
    {
        std::sort(resourcesToRemove.begin(), resourcesToRemove.end(),
                  [](const Res &a, const Res &b){
            return a.a < b.a; // least recently used first
        });
        for (const Res &res : resourcesToRemove)
        {
            if (resourcesTryRemove(resources.resources[res.n]))
            {
                memUse -= res.m;
                if (memUse < trs)
                    break;
            }
        }
    }
}

void MapImpl::resourcesCheckInitialized()
{
    OPTICK_EVENT();
    std::time_t current = std::time(nullptr);
    for (const auto &it : resources.resources)
    {
        resourceUpdateStatistics(it.second);
        const std::shared_ptr<Resource> &r = it.second;
        if (r->lastAccessTick + 3 < renderTickIndex)
            continue; // skip resources that were not accessed last tick
        switch ((Resource::State)r->state)
        {
        case Resource::State::errorRetry:
            if (r->retryNumber >= options.maxFetchRetries)
            {
                LOG(err3) << "All retries for resource <"
                    << r->name << "> has failed";
                r->state = Resource::State::errorFatal;
                statistics.resourcesFailed++;
                break;
            }
            if (r->retryTime == -1)
            {
                r->retryTime = (1 << r->retryNumber)
                    * options.fetchFirstRetryTimeOffset + current;
                LOGR(r->retryNumber < 2 ? dbglog::warn1 : dbglog::warn2)
                    << "Resource <" << r->name
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
            UTILITY_FALLTHROUGH;
        case Resource::State::initializing:
            r->state = Resource::State::checkCache;
            resources.queCacheRead.push(r);
            break;
        default:
            break;
        }
    }
}

void MapImpl::resourcesStartDownloads()
{
    OPTICK_EVENT();
    if (resources.downloads >= options.maxConcurrentDownloads)
    {
        for (const auto &it : resources.resources)
            resourceUpdateStatistics(it.second);
        return; // early exit
    }
    std::vector<std::weak_ptr<Resource>> res;
    for (const auto &it : resources.resources)
    {
        resourceUpdateStatistics(it.second);
        const std::shared_ptr<Resource> &r = it.second;
        if (r->state == Resource::State::startDownload)
            res.push_back(r);
    }
    {
        std::lock_guard<std::mutex> lock(resources.fetching.mut);
        res.swap(resources.fetching.resources);
    }
    resources.fetching.con.notify_one();
}

void MapImpl::resourceUpdateStatistics(const std::shared_ptr<Resource> &r)
{
    switch ((Resource::State)r->state)
    {
    case Resource::State::initializing:
    case Resource::State::checkCache:
    case Resource::State::startDownload:
    case Resource::State::downloading:
    case Resource::State::downloaded:
    case Resource::State::decoded:
        statistics.resourcesPreparing++;
        break;
    case Resource::State::ready:
    case Resource::State::errorFatal:
    case Resource::State::errorRetry:
    case Resource::State::availFail:
        break;
    }
}

void MapImpl::resourceFinalize()
{
    resources.queUpload.terminate();
    resources.resources.clear();
}

void MapImpl::resourceUpdate()
{
    OPTICK_EVENT();
    // resourcesPreparing is used to determine mapRenderComplete
    //   and must be updated every frame
    statistics.resourcesPreparing = 0;
    statistics.resourcesActive
        = resources.resources.size();
    statistics.resourcesDownloading
        = resources.downloads;
    statistics.resourcesQueueCacheRead
        = resources.queCacheRead.estimateSize();
    statistics.resourcesQueueCacheWrite
        = resources.queCacheWrite.estimateSize();
    statistics.resourcesQueueDecode
        = resources.queDecode.estimateSize();
    statistics.resourcesQueueUpload
        = resources.queUpload.estimateSize();
    statistics.resourcesQueueGeodata
        = resources.queGeodata.estimateSize();
    statistics.resourcesQueueAtmosphere
        = resources.queAtmosphere.estimateSize();

    // split workload into multiple render frames
    switch (renderTickIndex % 3)
    {
    case 0: return resourcesRemoveOld();
    case 1: return resourcesCheckInitialized();
    case 2: return resourcesStartDownloads();
    }
}

} // namespace vts
