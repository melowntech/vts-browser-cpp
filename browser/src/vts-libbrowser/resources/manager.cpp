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

#include "../include/vts-browser/log.hpp"
#include "../map.hpp"

namespace vts
{

////////////////////////////
// DATA THREAD
////////////////////////////

void MapImpl::resourceUploadProcess(const std::shared_ptr<Resource> &r)
{
    assert(r->state == Resource::State::downloaded);
    r->info.gpuMemoryCost = r->info.ramMemoryCost = 0;
    statistics.resourcesProcessed++;
    try
    {
        r->load();
        r->state = Resource::State::ready;
    }
    catch (const std::exception &e)
    {
        LOG(err3) << "Failed processing resource <" << r->name
            << ">, exception <" << e.what() << ">";
        if (options.debugSaveCorruptedFiles)
        {
            std::string path = std::string() + "corrupted/"
                + convertNameToPath(r->name, false);
            try
            {
                writeLocalFileBuffer(path, r->fetch->reply.content);
                LOG(info1) << "Resource <" << r->name
                    << "> saved into file <"
                    << path << "> for further inspection";
            }
            catch (...)
            {
                LOG(warn1) << "Failed saving resource <" << r->name
                    << "> into file <"
                    << path << "> for further inspection";
            }
        }
        statistics.resourcesFailed++;
        r->state = Resource::State::errorFatal;
    }
    r->fetch.reset();
}

void MapImpl::resourceDataInitialize()
{
    LOG(info3) << "Data initialize";
}

void MapImpl::resourceDataFinalize()
{
    LOG(info3) << "Data finalize";
}

void MapImpl::resourceDataTick()
{
    statistics.dataTicks = ++resources.tickIndex;

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
    LOGTHROW(fatal, std::runtime_error) << "Map::DataRun() not yet implemented";
}

////////////////////////////
// CACHE WRITE THREAD
////////////////////////////

void MapImpl::cacheWriteEntry()
{
    setLogThreadName("cache writer");
    while (!resources.queCacheWrite.stopped())
    {
        CacheWriteData cwd;
        resources.queCacheWrite.waitPop(cwd);
        if (!cwd.name.empty())
            resources.cache->write(cwd.name, cwd.buffer, cwd.expires);
    }
}

////////////////////////////
// CACHE READ THREAD
////////////////////////////

void MapImpl::cacheReadEntry()
{
    setLogThreadName("cache reader");
    while (!resources.queCacheRead.stopped())
    {
        std::shared_ptr<Resource> r;
        resources.queCacheRead.waitPop(r);
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
    assert(r->state == Resource::State::checkCache);
    r->fetch = std::make_shared<FetchTaskImpl>(r);
    r->info.gpuMemoryCost = r->info.ramMemoryCost = 0;
    if (r->allowDiskCache() && resources.cache->read(
        r->name, r->fetch->reply.content,
        r->fetch->reply.expires))
    {
        statistics.resourcesDiskLoaded++;
        if (r->fetch->reply.content.size() > 0)
            r->state = Resource::State::downloaded;
        else
            r->state = Resource::State::availFail;
        r->fetch->reply.code = 200;
    }
    else if (startsWith(r->name, "file://"))
    {
        r->fetch->reply.content = readLocalFileBuffer(r->name.substr(7));
        r->state = Resource::State::downloaded;
        r->fetch->reply.code = 200;
    }
    else if (startsWith(r->name, "internal://"))
    {
        r->fetch->reply.content = readInternalMemoryBuffer(r->name.substr(11));
        r->state = Resource::State::downloaded;
        r->fetch->reply.code = 200;
    }
    else
    {
        r->state = Resource::State::startDownload;
        // will be handled in main thread
    }

    if (r->state == Resource::State::downloaded)
        resources.queUpload.push(r);
}

////////////////////////////
// MAIN THREAD
////////////////////////////

void MapImpl::resourcesRemoveOld()
{
    struct Res
    {
        std::string s; // name
        uint32 p; // lastAccessTick
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
    uint64 trs = (uint64)options.targetResourcesMemoryKB * 1024;
    if (memUse > trs)
    {
        std::sort(resToRemove.begin(), resToRemove.end());
        for (Res &res : resToRemove)
        {
            auto &it = resources.resources[res.s];
            std::string name = it->name;
            uint64 mem = it->info.gpuMemoryCost + it->info.ramMemoryCost;
            {
                // release the pointer if we are last holding it
                std::weak_ptr<Resource> w = it;
                it.reset();
                it = w.lock();
            }
            if (!it)
            {
                LOG(info1) << "Released resource <" << name << ">";
                resources.resources.erase(name);
                statistics.resourcesReleased++;
                memUse -= mem;
                if (memUse <= trs)
                    break;
            }
        }
    }
    statistics.currentGpuMemUseKB = memGpuUse / 1024;
    statistics.currentRamMemUseKB = memRamUse / 1024;
    statistics.resourcesActive = resources.resources.size();
    statistics.resourcesDownloading = resources.downloads;
}

void MapImpl::resourcesCheckInitialized()
{
    std::time_t current = std::time(nullptr);
    for (auto it : resources.resources)
    {
        const std::shared_ptr<Resource> &r = it.second;
        if (r->lastAccessTick + 1 != renderer.tickIndex)
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
            UTILITY_FALLTHROUGH;
        case Resource::State::initializing:
            r->state = Resource::State::checkCache;
            resources.queCacheRead.push(r);
            break;
        }
    }
}

void MapImpl::resourcesStartDownloads()
{
    if (resources.downloads >= options.maxConcurrentDownloads)
        return; // early exit
    std::vector<std::shared_ptr<Resource>> res;
    for (auto it : resources.resources)
    {
        const std::shared_ptr<Resource> &r = it.second;
        if (r->lastAccessTick + 1 != renderer.tickIndex)
            continue; // skip resources that were not accessed last tick
        if (r->state == Resource::State::startDownload)
            res.push_back(r);
    }
    // sort candidates by priority
    std::sort(res.begin(), res.end(), [](auto &a, auto &b) {
        return a->priority > b->priority;
    });
    // reset the priority
    for (auto &r : res)
        if (r->priority < std::numeric_limits<float>::infinity())
            r->priority = 0;
    for (auto &r : res)
    {
        if (resources.downloads >= options.maxConcurrentDownloads)
            break;
        resources.downloads++;
        LOG(debug) << "Initializing fetch of <" << r->name << ">";
        r->fetch->query.headers["X-Vts-Client-Id"] = createOptions.clientId;
        if (resources.auth)
            resources.auth->authorize(r);
        //if (r->fetch->query.url.empty())
        //    r->fetch->query.url = r->name;
        r->state = Resource::State::downloading;
        resources.fetcher->fetch(r->fetch);
        statistics.resourcesDownloaded++;
    }
}

void MapImpl::resourceRenderInitialize()
{}

void MapImpl::resourceRenderFinalize()
{
    resources.queUpload.terminate();
    resources.resources.clear();
}

void MapImpl::resourceRenderTick()
{
    switch (renderer.tickIndex % 3)
    {
    case 0: return resourcesRemoveOld();
    case 1: return resourcesCheckInitialized();
    case 2: return resourcesStartDownloads();
    }
}

} // namespace vts
