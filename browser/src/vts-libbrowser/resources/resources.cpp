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

#include "../fetchTask.hpp"
#include "../map.hpp"
#include "../authConfig.hpp"
#include "../resources.hpp"
#include "../utilities/dataUrl.hpp"

#include <optick.h>

#include <thread>
#include <chrono>

namespace vts
{

    void Resources::resourceSaveCorruptedFile(const std::shared_ptr<Resource> &r)
    {
        if (!map->options.debugSaveCorruptedFiles)
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

    namespace
    {

        typedef std::pair<float, std::shared_ptr<Resource>> ResourceWithPriority;

        std::vector<ResourceWithPriority> filterSortResources(
            const std::vector<std::weak_ptr<Resource>> &resources,
            Resource::State requiredState)
        {
            std::vector<ResourceWithPriority> res;
            for (const auto &w : resources)
            {
                std::shared_ptr<Resource> r = w.lock();
                if (r)
                {
                    if (r->state != requiredState)
                        continue;
                    res.emplace_back(r->priority, r);
                    if (r->priority < inf1())
                        r->priority = 0;
                }
            }
            std::sort(res.begin(), res.end(),
                [](ResourceWithPriority &a, ResourceWithPriority &b) {
                    return a.first > b.first; // highest priority first
                });
            return res;
        }

    } // namespace

    UploadData::UploadData()
    {}

    UploadData::UploadData(const std::shared_ptr<Resource> &resource)
        : uploadData(resource)
    {}

    UploadData::UploadData(std::shared_ptr<void> &userData, int)
    {
        std::swap(userData, destroyData);
    }

    void UploadData::process()
    {
        destroyData.reset();
        auto r = uploadData.lock();
        if (r)
            r->map->resources->resourceUploadProcess(r);
    }

    ////////////////////////////
    // A FETCH THREAD
    ////////////////////////////

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
        map->resources->downloads--;
        map->resources->downloadsCondition.notify_one();
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
        if ((state == Resource::State::availFail
            || state == Resource::State::downloading)
            && map->resources->queCacheWrite.estimateSize()
            < map->options.maxCacheWriteQueueLength)
        {
            // this makes copy of the content buffer
            map->resources->queCacheWrite.push(CacheData(this,
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
                    map->resources->queDecode.push(rs);
                }
            }
        }
    }

    ////////////////////////////
    // DECODE THREAD
    ////////////////////////////

    void Resources::resourceDecodeProcess(const std::shared_ptr<Resource> &r)
    {
        OPTICK_EVENT();
        OPTICK_TAG("name", r->name.c_str());

        assert(r->state == Resource::State::downloaded);
        map->statistics.resourcesDecoded++;
        r->info.gpuMemoryCost = r->info.ramMemoryCost = 0;
        try
        {
            r->decode();
            if (r->requiresUpload())
            {
                r->state = Resource::State::decoded;
                queUpload.push(UploadData(r));
            }
            else
                r->state = Resource::State::ready;
        }
        catch (const std::exception &e)
        {
            LOG(err3) << "Failed decoding resource <" << r->name
                << ">, exception <" << e.what() << ">";
            resourceSaveCorruptedFile(r);
            map->statistics.resourcesFailed++;
            r->state = Resource::State::errorFatal;
        }
        r->fetch.reset();
    }

    void Resources::resourcesDecodeProcessorEntry()
    {
        OPTICK_THREAD("decode");
        setLogThreadName("decode");
        while (!queDecode.stopped())
        {
            std::weak_ptr<Resource> w;
            queDecode.waitPop(w);
            std::shared_ptr<Resource> r = w.lock();
            if (!r)
                continue;
            resourceDecodeProcess(r);
        }
    }

    ////////////////////////////
    // DATA THREAD
    ////////////////////////////

    void Resources::resourceUploadProcess(const std::shared_ptr<Resource> &r)
    {
        OPTICK_EVENT();
        OPTICK_TAG("name", r->name.c_str());

        assert(r->state == Resource::State::decoded);
        map->statistics.resourcesUploaded++;
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
            map->statistics.resourcesFailed++;
            r->state = Resource::State::errorFatal;
        }
        r->decodeData.reset();
    }

    void Resources::resourcesUploadProcessorEntry()
    {
        OPTICK_THREAD("data");
        while (!queUpload.stopped())
        {
            UploadData w;
            queUpload.waitPop(w);
            w.process();
        }
    }

    void Resources::resourcesDataUpdate()
    {
        OPTICK_EVENT();
        for (uint32 proc = 0; proc < map->options
            .maxResourceProcessesPerTick; proc++)
        {
            UploadData w;
            if (queUpload.tryPop(w))
                w.process();
            else
                break;
        }
    }

    void Resources::resourcesDataFinalize()
    {
        assert(queUpload.stopped());
        queUpload.purge();
    }

    void Resources::resourcesDataAllRun()
    {
        resourcesUploadProcessorEntry();
        resourcesDataFinalize();
    }

    ////////////////////////////
    // CACHE WRITE THREAD
    ////////////////////////////

    void Resources::cacheWriteEntry()
    {
        OPTICK_THREAD("cache writer");
        setLogThreadName("cache writer");
        while (!queCacheWrite.stopped())
        {
            CacheData cwd;
            queCacheWrite.waitPop(cwd);
            if (!cwd.name.empty())
                cacheWrite(std::move(cwd));
        }
    }

    ////////////////////////////
    // CACHE READ THREAD
    ////////////////////////////

    void Resources::cacheReadEntry()
    {
        OPTICK_THREAD("cache reader");
        setLogThreadName("cache reader");
        while (!queCacheRead.stopped())
        {
            auto res1 = queCacheRead.readAllWait();
            OPTICK_EVENT("update");
            auto res2 = filterSortResources(res1, Resource::State::checkCache);
            for (const auto &pr : res2)
            {
                const std::shared_ptr<Resource> &r = pr.second;
                try
                {
                    cacheReadProcess(r);
                }
                catch (const std::exception &e)
                {
                    map->statistics.resourcesFailed++;
                    r->state = Resource::State::errorFatal;
                    LOG(err3) << "Failed preparing resource <" << r->name
                        << ">, exception <" << e.what() << ">";
                }
                if (queCacheRead.estimateSize() > 0)
                    break; // refresh the priorities
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

    void Resources::cacheReadProcess(const std::shared_ptr<Resource> &r)
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
            map->statistics.resourcesDiskLoaded++;
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
            queDecode.push(r);
    }

    ////////////////////////////
    // FETCHER THREAD
    ////////////////////////////

    void Resources::resourcesDownloadsEntry()
    {
        OPTICK_THREAD("fetcher");
        setLogThreadName("fetcher");
        map->fetcher->initialize();
        std::mutex dummyMutex;
        while (!queFetching.stopped())
        {
            auto res1 = queFetching.readAllWait();
            OPTICK_EVENT("update");
            map->fetcher->update();
            auto res2 = filterSortResources(res1, Resource::State::startDownload);
            for (const auto &pr : res2)
            {
                while (downloads >= map->options.maxConcurrentDownloads)
                {
                    std::unique_lock<std::mutex> lock(dummyMutex);
                    downloadsCondition.wait(lock);
                }
                const std::shared_ptr<Resource> &r = pr.second;
                r->state = Resource::State::downloading;
                downloads++;
                LOG(debug) << "Initializing fetch of <" << r->name << ">";
                r->fetch->query.headers["X-Vts-Client-Id"]
                    = map->createOptions.clientId;
                if (map->auth)
                    map->auth->authorize(r);
                map->fetcher->fetch(r->fetch);
                map->statistics.resourcesDownloaded++;
                if (queFetching.estimateSize() > 0)
                    break; // refresh the priorities
            }
        }
        map->fetcher->finalize();
        map->fetcher.reset();
    }

    ////////////////////////////
    // MAIN THREAD
    ////////////////////////////

    Resources::Resources(MapImpl *map) : map(map)
    {
        thrFetcher
            = std::thread(&Resources::resourcesDownloadsEntry, this);
        thrCacheReader
            = std::thread(&Resources::cacheReadEntry, this);
        thrCacheWriter
            = std::thread(&Resources::cacheWriteEntry, this);
        thrDecoder
            = std::thread(&Resources::resourcesDecodeProcessorEntry, this);
        thrGeodataProcessor
            = std::thread(&Resources::resourcesGeodataProcessorEntry, this);
        thrAtmosphereGenerator
            = std::thread(&Resources::resourcesAtmosphereGeneratorEntry, this);
        cacheInit();
    }

    Resources::~Resources()
    {
        resourcesTerminateAllQueues();
        thrFetcher.join();
        thrCacheReader.join();
        thrCacheWriter.join();
        thrDecoder.join();
        thrAtmosphereGenerator.join();
        thrGeodataProcessor.join();
    }

    bool Resources::resourcesTryRemove(std::shared_ptr<Resource> &r)
    {
        std::string name = r->name;
        assert(resources.count(name) == 1);
        {
            // release the pointer if we are the last one holding it
            std::weak_ptr<Resource> w = r;
#ifdef NDEBUG
            r.reset();
#else
            try
            {
                r.reset();
            }
            catch (...)
            {
                LOGTHROW(fatal, std::logic_error)
                    << "Exception in destructor";
            }
#endif // NDEBUG
            r = w.lock();
        }
        if (!r)
        {
            LOG(info1) << "Released resource <" << name << ">";
            resources.erase(name);
            map->statistics.resourcesReleased++;
            return true;
        }
        return false;
    }

    void Resources::resourcesRemoveOld()
    {
        OPTICK_EVENT();
        struct Res
        {
            const std::string *n; // name
            uint32 m; // memory used
            uint32 a; // lastAccessTick
            Res(const std::string *n, uint32 m, uint32 a) : n(n), m(m), a(a)
            {}
        };
        // successfully loaded resources are removed
        //   only when we are tight on memory
        std::vector<Res> resourcesToRemove;
        resourcesToRemove.reserve(resources.size() / 4);
        // resources that errored are removed immediately
        std::vector<Res> unconditionalToRemove;
        unconditionalToRemove.reserve(resources.size() / 4);
        uint64 memRamUse = 0;
        uint64 memGpuUse = 0;
        // find resource candidates for removal
        for (const auto &it : resources)
        {
            memRamUse += it.second->info.ramMemoryCost;
            memGpuUse += it.second->info.gpuMemoryCost;
            // skip recently used resources
            if (it.second->lastAccessTick + 5 < map->renderTickIndex)
            {
                Res r(&it.second->name,
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
        map->statistics.currentGpuMemUseKB = memGpuUse / 1024;
        map->statistics.currentRamMemUseKB = memRamUse / 1024;
        uint64 memUse = memRamUse + memGpuUse;
        // remove unconditionalToRemove
        for (const Res &res : unconditionalToRemove)
        {
            if (resourcesTryRemove(resources[*res.n]))
                memUse -= res.m;
        }
        // remove resourcesToRemove
        uint64 trs = (uint64)map->options.targetResourcesMemoryKB * 1024;
        if (memUse > trs)
        {
            std::sort(resourcesToRemove.begin(), resourcesToRemove.end(),
                [](const Res &a, const Res &b) {
                    return a.a < b.a; // least recently used first
                });
            for (const Res &res : resourcesToRemove)
            {
                if (resourcesTryRemove(resources[*res.n]))
                {
                    memUse -= res.m;
                    if (memUse < trs)
                        break;
                }
            }
        }
    }

    void Resources::resourcesCheckInitialized()
    {
        OPTICK_EVENT();
        std::time_t current = std::time(nullptr);

        for (const auto &it : resources)
        {
            const std::shared_ptr<Resource> &r = it.second;
            if (r->lastAccessTick + 3 < map->renderTickIndex)
                continue; // skip resources that were not accessed last tick
            switch ((Resource::State)r->state)
            {
            case Resource::State::errorRetry:
                if (r->retryNumber >= map->options.maxFetchRetries)
                {
                    LOG(err3) << "All retries for resource <"
                        << r->name << "> has failed";
                    r->state = Resource::State::errorFatal;
                    map->statistics.resourcesFailed++;
                    break;
                }
                if (r->retryTime == -1)
                {
                    r->retryTime = (1 << r->retryNumber)
                        * map->options.fetchFirstRetryTimeOffset + current;
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
                break;
            default:
                break;
            }
        }
    }

    void Resources::resourcesStartDownloads()
    {
        OPTICK_EVENT();
        std::vector<std::weak_ptr<Resource>> requestCacheRead;
        std::vector<std::weak_ptr<Resource>> requestDownloads;

        for (const auto &it : resources)
        {
            const std::shared_ptr<Resource> &r = it.second;
            switch ((Resource::State)r->state)
            {
            case Resource::State::checkCache:
                requestCacheRead.push_back(r);
                break;
            case Resource::State::startDownload:
                requestDownloads.push_back(r);
                break;
            default:
                break;
            }
        }

        map->statistics.resourcesQueueCacheRead = requestCacheRead.size();
        queCacheRead.writeAll(requestCacheRead);
        map->statistics.resourcesQueueDownload = requestDownloads.size();
        queFetching.writeAll(requestDownloads);
    }

    void Resources::resourcesTerminateAllQueues()
    {
        queCacheWrite.terminate();
        queDecode.terminate();
        queUpload.terminate();
        queAtmosphere.terminate();
        queGeodata.terminate();
        queCacheRead.terminate();
        queFetching.terminate();
    }

    void Resources::resourcesRenderFinalize()
    {
        OPTICK_EVENT();

        // release resources held by the map and all layers
        map->purgeMapconfig();

        // clear the resources now while all the necessary things are still working
        resources.clear();

        // allow the dataAllRun method to return to the caller
        resourcesTerminateAllQueues();
    }

    void Resources::resourcesRenderUpdate()
    {
        OPTICK_EVENT();

        {
            OPTICK_EVENT("statistics");

            // resourcesPreparing is used to determine mapRenderComplete
            //   and must be updated every frame
            map->statistics.resourcesPreparing = 0;
            for (const auto &it : resources)
            {
                switch ((Resource::State)it.second->state)
                {
                case Resource::State::initializing:
                case Resource::State::checkCache:
                case Resource::State::startDownload:
                case Resource::State::downloading:
                case Resource::State::downloaded:
                case Resource::State::decoded:
                    map->statistics.resourcesPreparing++;
                    break;
                case Resource::State::ready:
                case Resource::State::errorFatal:
                case Resource::State::errorRetry:
                case Resource::State::availFail:
                    break;
                }
            }

            map->statistics.resourcesActive
                = resources.size();
            map->statistics.resourcesDownloading
                = downloads;
            map->statistics.resourcesQueueCacheWrite
                = queCacheWrite.estimateSize();
            map->statistics.resourcesQueueDecode
                = queDecode.estimateSize();
            map->statistics.resourcesQueueUpload
                = queUpload.estimateSize();
            map->statistics.resourcesQueueGeodata
                = queGeodata.estimateSize();
            map->statistics.resourcesQueueAtmosphere
                = queAtmosphere.estimateSize();
        }

        // split workload into multiple render frames
        switch (map->renderTickIndex % 3)
        {
        case 0: return resourcesRemoveOld();
        case 1: return resourcesCheckInitialized();
        case 2: return resourcesStartDownloads();
        }
    }

} // namespace vts
