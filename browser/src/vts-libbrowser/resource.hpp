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

#ifndef RESOURCE_HPP_fh4st78j4df9
#define RESOURCE_HPP_fh4st78j4df9

#include <memory>
#include <string>
#include <atomic>
#include <ctime>

#include "include/vts-browser/resources.hpp"
#include "include/vts-browser/fetcher.hpp"

namespace vts
{

class MapImpl;
class FetchTaskImpl;

class Resource : public std::enable_shared_from_this<Resource>,
        private Immovable
{
public:
    enum class State
    {
        initializing,
        checkCache,
        startDownload,
        downloading,
        downloaded,
        decoded,
        ready,
        errorFatal,
        errorRetry,
        availFail,
    };

    Resource(MapImpl *map, const std::string &name);
    Resource(const Resource &other) = delete;
    Resource &operator = (const Resource &other) = delete;
    virtual ~Resource();
    virtual void decode() = 0; // eg. decode an image
    virtual void upload() {} // call the resource callback
    virtual FetchTask::ResourceType resourceType() const = 0;
    bool allowDiskCache() const;
    static bool allowDiskCache(FetchTask::ResourceType type);
    void updatePriority(float priority);
    void updateAvailability(const std::shared_ptr<void> &availTest);
    void forceRedownload();
    explicit operator bool() const; // return state == ready

    const std::string name;
    MapImpl *const map;
    std::atomic<State> state;
    ResourceInfo info;
    std::shared_ptr<void> decodeData;
    std::shared_ptr<FetchTaskImpl> fetch;
    std::time_t retryTime;
    uint32 retryNumber;
    uint32 lastAccessTick;
    float priority;
};

std::ostream &operator << (std::ostream &stream, Resource::State state);
bool testAndThrow(Resource::State state, const std::string &message);

} // namespace vts

#endif
