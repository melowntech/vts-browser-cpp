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

#include "../resources.hpp"
#include "../resource.hpp"
#include "../fetchTask.hpp"
#include "../map.hpp"

namespace vts
{

FetchTaskImpl::FetchTaskImpl(const std::shared_ptr<Resource> &resource) : FetchTask(resource->name, resource->resourceType()), name(resource->name), map(resource->map), resource(resource)
{
    reply.expires = -1;
}

bool FetchTaskImpl::performAvailTest() const
{
    if (!availTest)
        return true;
    auto at = std::static_pointer_cast<vtslibs::registry::BoundLayer::Availability>(availTest);
    switch (at->type)
    {
    case vtslibs::registry::BoundLayer::Availability::Type::negativeCode:
        if (at->codes.find(reply.code) == at->codes.end())
            return false;
        break;
    case vtslibs::registry::BoundLayer::Availability::Type::negativeType:
        if (at->mime == reply.contentType)
            return false;
        break;
    case vtslibs::registry::BoundLayer::Availability::Type::negativeSize:
        if (reply.content.size() <= (unsigned)at->size)
            return false;
        break;
    }
    return true;
}

Resource::Resource(vts::MapImpl *map, const std::string &name) : name(name), map(map), priority(nan1())
{
    LOG(debug) << "Constructing resource <" << name << "> at <" << this << ">";
    map->resources->existing++;
}

Resource::~Resource()
{
    LOG(debug) << "Destroying resource <" << name << "> at <" << this << ">";
    if (info.userData)
    {
        assert(!map->resources->queUpload.stop);
        map->resources->queUpload.push(UploadData(info.userData, 0));
    }
    map->resources->existing--;
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
    case FetchTask::ResourceType::Mapconfig:
    case FetchTask::ResourceType::BoundLayerConfig:
    case FetchTask::ResourceType::FreeLayerConfig:
    case FetchTask::ResourceType::TilesetMappingConfig:
        return false;
    default:
        return true;
    }
}

void Resource::updatePriority(float p)
{
    if (!std::isnan(priority))
        priority = std::max(priority, p);
    else
        priority = p;
}

void Resource::updateAvailability(const std::shared_ptr<void> &availTest)
{
    auto f = fetch;
    if (f)
    {
        if (!f->availTest)
            f->availTest = availTest;
    }
    else
    {
        f = std::make_shared<FetchTaskImpl>(map->resources->resources[name]);
        f->availTest = availTest;
        fetch = f;
    }
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

std::shared_ptr<void> Resource::getUserData() const
{
    return std::shared_ptr<void>(shared_from_this(), info.userData.get());
}

std::ostream &operator << (std::ostream &stream, Resource::State state)
{
    switch (state)
    {
        case Resource::State::initializing:
            stream << "initializing";
            break;
        case Resource::State::cacheReadQueue:
            stream << "cacheReadQueue";
            break;
        case Resource::State::fetchQueue:
            stream << "fetchQueue";
            break;
        case Resource::State::fetching:
            stream << "fetching";
            break;
        case Resource::State::decodeQueue:
            stream << "decodeQueue";
            break;
        case Resource::State::uploadQueue:
            stream << "uploadQueue";
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
    case GpuTypeEnum::HalfFloat:
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
