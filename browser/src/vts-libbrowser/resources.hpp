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

#ifndef RESOURCES_HPP_zujhgfw89e7rjk7
#define RESOURCES_HPP_zujhgfw89e7rjk7

#include <unordered_map>
#include <atomic>
#include <thread>
#include <memory>

#include "../include/vts-browser/buffer.hpp"

#include "../utilities/threadQueue.hpp"
#include "../validity.hpp"

namespace vts
{

class MapImpl;
class Resource;
class Fetcher;
class Cache;
class AuthConfig;
class SearchTask;
class SearchTaskImpl;
class FetchTaskImpl;
class GeodataTile;
class GpuAtmosphereDensityTexture;

class CacheData
{
public:
    CacheData() = default;
    CacheData(FetchTaskImpl *task, bool availFailed = false);

    Buffer buffer;
    std::string name;
    sint64 expires = 0;
    bool availFailed = false;
};

class UploadData
{
public:
    UploadData();
    explicit UploadData(const std::shared_ptr<Resource> &resource); // upload
    explicit UploadData(std::shared_ptr<void> &userData, int); // destroy
    UploadData(const UploadData &) = delete;
    UploadData(UploadData &&) = default;
    UploadData &operator = (const UploadData &) = delete;
    UploadData &operator = (UploadData &&) = default;

    void process();

protected:
    std::weak_ptr<Resource> uploadData;
    std::shared_ptr<void> destroyData;
};

class Resources : private Immovable
{
public:
    MapImpl *const map;

    std::unordered_map<std::string, std::shared_ptr<Resource>> resources;
    std::atomic<uint32> downloads {0}; // number of active downloads
    std::condition_variable downloadsCondition;

    ThreadQueue<std::weak_ptr<Resource>> queFetching;
    ThreadQueue<std::weak_ptr<Resource>> queCacheRead;
    ThreadQueue<CacheData> queCacheWrite;
    ThreadQueue<std::weak_ptr<Resource>> queDecode;
    ThreadQueue<std::weak_ptr<GeodataTile>> queGeodata;
    ThreadQueue<std::weak_ptr<GpuAtmosphereDensityTexture>> queAtmosphere;
    ThreadQueue<UploadData> queUpload;
    std::thread thrFetcher;
    std::thread thrCacheReader;
    std::thread thrCacheWriter;
    std::thread thrDecoder;
    std::thread thrGeodataProcessor;
    std::thread thrAtmosphereGenerator;

    Resources(MapImpl *map);
    ~Resources();

    void resourcesDataFinalize();
    void resourcesRenderFinalize();
    void resourcesDataUpdate();
    void resourcesRenderUpdate();
    void resourcesDataAllRun();
    void purgeResourcesCache();

    bool resourcesTryRemove(std::shared_ptr<Resource> &r);
    void resourcesRemoveOld();
    void resourcesCheckInitialized();
    void resourcesStartDownloads();
    void resourcesDownloadsEntry();
    void resourcesUploadProcessorEntry();
    void resourcesAtmosphereGeneratorEntry();
    void resourcesGeodataProcessorEntry();
    void resourcesDecodeProcessorEntry();
    void resourceDecodeProcess(const std::shared_ptr<Resource> &r);
    void resourceUploadProcess(const std::shared_ptr<Resource> &r);
    void resourceSaveCorruptedFile(const std::shared_ptr<Resource> &r);
    void resourcesTerminateAllQueues();

    void cacheInit();
    void cacheWriteEntry();
    void cacheWrite(CacheData &&data);
    void cacheReadEntry();
    void cacheReadProcess(const std::shared_ptr<Resource> &r);
    CacheData cacheRead(const std::string &name);
};

} // namespace vts

#endif
