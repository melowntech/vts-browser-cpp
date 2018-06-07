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

#include "../map.hpp"

namespace vts
{

FetchTaskImpl::FetchTaskImpl(const std::shared_ptr<Resource> &resource) :
	FetchTask(resource->name, resource->resourceType()),
    name(resource->name), map(resource->map), resource(resource), redirectionsCount(0)
{
    reply.expires = -1;
}

void FetchTaskImpl::fetchDone()
{
    ////////////////////////////
    // SOME FETCH THREAD
    ////////////////////////////

    LOG(debug) << "Resource <" << name << "> finished downloading";
    assert(map);
    map->resources.downloads--;
    Resource::State state = Resource::State::downloading;

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
    if (!Resource::allowDiskCache(query.resourceType))
        reply.expires = -2;

    // availability tests
    if (state == Resource::State::downloading)
    {
        std::shared_ptr<Resource> rs = resource.lock();
        if (rs && !rs->performAvailTest())
        {
            LOG(info1) << "Resource <" << name
                << "> failed availability test";
            state = Resource::State::availFail;
        }
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
            std::string().swap(reply.redirectUrl);
            LOG(info1) << "Download of <"
                << name << "> redirected to <"
                << query.url << ">, http code " << reply.code;
            reply = Reply();
            state = Resource::State::initializing;
        }
    }

    // update the actual resource
    if (state == Resource::State::downloading)
        state = Resource::State::downloaded;
    else
        reply.content.free();
    {
        std::shared_ptr<Resource> rs = resource.lock();
        if (rs)
        {
            assert(&*rs->fetch == this);
            assert(rs->state == Resource::State::downloading);
            rs->info.ramMemoryCost = reply.content.size();
            rs->state = state;
            map->resources.queUpload.push(rs);
        }
    }

    // (deferred) write to cache
    if (state == Resource::State::availFail 
        || state == Resource::State::downloaded)
    {
        CacheWriteData c;
        c.name = name;
        c.buffer = reply.content.copy();
        c.expires = reply.expires;
        map->resources.queCacheWrite.push(std::move(c));
    }
}

ResourceInfo::ResourceInfo() :
    ramMemoryCost(0), gpuMemoryCost(0)
{}

Resource::Resource(vts::MapImpl *map, const std::string &name) :
    name(name), map(map),
    state(State::initializing),
    retryTime(-1), retryNumber(0),
    lastAccessTick(0),
    priority(std::numeric_limits<float>::quiet_NaN())
{
    LOG(debug) << "Constructing resource <" << name
               << "> at <" << this << ">";
}

Resource::~Resource()
{
    LOG(debug) << "Destroying resource <" << name
               << "> at <" << this << ">";
}

bool Resource::allowDiskCache() const
{
    return allowDiskCache(resourceType());
}

bool Resource::allowDiskCache(FetchTask::ResourceType type)
{
    switch (type)
    {
    case FetchTask::ResourceType::AuthConfig:
    case FetchTask::ResourceType::MapConfig:
    case FetchTask::ResourceType::BoundLayerConfig:
    case FetchTask::ResourceType::TilesetMappingConfig:
        return false;
    default:
        return true;
    }
}

bool Resource::performAvailTest() const
{
    if (!availTest)
        return true;
    switch (availTest->type)
    {
    case vtslibs::registry::BoundLayer::Availability::Type::negativeCode:
        if (availTest->codes.find(fetch->reply.code) == availTest->codes.end())
            return false;
        break;
    case vtslibs::registry::BoundLayer::Availability::Type::negativeType:
        if (availTest->mime == fetch->reply.contentType)
            return false;
        break;
    case vtslibs::registry::BoundLayer::Availability::Type::negativeSize:
        if (fetch->reply.content.size() <= (unsigned)availTest->size)
            return false;
        break;
    }
    return true;
}

void Resource::updatePriority(float p)
{
    if (priority == priority)
        priority = std::max(priority, p);
    else
        priority = p;
}

void Resource::forceRedownload()
{
    retryNumber = 0;
    state = Resource::State::errorRetry;
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
        case Resource::State::checkCache:
            stream << "readCache";
            break;
        case Resource::State::startDownload:
            stream << "startDownload";
            break;
        case Resource::State::downloading:
            stream << "downloading";
            break;
        case Resource::State::downloaded:
            stream << "downloaded";
            break;
        case Resource::State::uploading:
            stream << "uploading";
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
        default:
            LOGTHROW(fatal, std::invalid_argument) << "invalid resource state enum";
            throw;
    }
    return stream;
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

} // namespace vts
