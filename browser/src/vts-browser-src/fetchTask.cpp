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

#include "map.hpp"

namespace vts
{

namespace
{

void initializeFetchTask(MapImpl *map, FetchTaskImpl *task)
{
    task->queryHeaders["X-Vts-Client-Id"] = map->clientId;
    if (map->auth)
        map->auth->authorize(task);
}

} // namespace

FetchTask::FetchTask(const std::string &url, ResourceType resourceType) :
    resourceType(resourceType), queryUrl(url), replyCode(0)
{}

FetchTask::~FetchTask()
{}

FetchTaskImpl::FetchTaskImpl(MapImpl *map, const std::string &name,
                             FetchTask::ResourceType resourceType) :
    FetchTask(name, resourceType), name(name), map(map),
    state(State::initializing), redirectionsCount(0)
{
    initializeFetchTask(map, this);
}

FetchTaskImpl::~FetchTaskImpl()
{}

bool FetchTaskImpl::performAvailTest() const
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
    default:
        LOGTHROW(fatal, std::invalid_argument) << "Invalid available test type";
    }
    return true;
}

bool FetchTaskImpl::allowDiskCache() const
{
    if (map->resources.disableCache)
        return false;
    switch (resourceType)
    {
    case ResourceType::AuthConfig:
    case ResourceType::MapConfig:
    case ResourceType::BoundLayerConfig:
        return false;
    default:
        return true;
    }
}

void FetchTaskImpl::saveToCache()
{
    assert(!map->resources.disableCache);
    assert(replyCode == 200);
    try
    {
        writeLocalFileBuffer(map->convertNameToCache(name), contentData);
    }
    catch (...)
    {
		// do nothing
	}
}

bool FetchTaskImpl::loadFromCache()
{
    assert(allowDiskCache());
    try
    {
        std::string path = map->convertNameToCache(name);
        if (!boost::filesystem::exists(path))
            return false;
        contentData = readLocalFileBuffer(path);
        replyCode = 200;
        return true;
    }
    catch (...)
    {
        LOG(err2) << "Error reading resource '"
                  << name << "' from cache file";
		replyCode = 404;
    }
    return false;
}

void FetchTaskImpl::loadFromInternalMemory()
{
    try
    {
        contentData = readInternalMemoryBuffer(name);
        replyCode = 200;
    }
    catch (...)
    {
        LOG(err2) << "Error reading resource '"
                  << name << "' from internal memory";
		replyCode = 404;
    }
}

void FetchTaskImpl::loadFromLocalFile()
{
    try
    {
        contentData = readLocalFileBuffer(name);
        replyCode = 200;
    }
    catch (...)
    {
        LOG(err2) << "Error reading resource '"
                  << name << "' from local file";
		replyCode = 404;
    }
}

void FetchTaskImpl::fetchDone()
{
    map->fetchedFile(this);
}

} // namespace vts
