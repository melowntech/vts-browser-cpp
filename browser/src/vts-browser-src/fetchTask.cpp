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

void FetchTaskImpl::fetchDone()
{
    map->fetchedFile(this);
}

} // namespace vts
