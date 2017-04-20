#include "map.hpp"

namespace vts
{

void writeLocalFileBuffer(const std::string &path, const Buffer &buffer)
{
    boost::filesystem::create_directories(
                boost::filesystem::path(path).parent_path());
    FILE *f = fopen(path.c_str(), "wb");
    if (!f)
        throw std::runtime_error("failed to write file");
    if (fwrite(buffer.data(), buffer.size(), 1, f) != 1)
    {
        fclose(f);
        throw std::runtime_error("failed to write file");
    }
    if (fclose(f) != 0)
        throw std::runtime_error("failed to write file");
}

FetchTask::FetchTask(const std::string &url) : url(url), 
    allowRedirects(false), code(0), redirectionsCount(0)
{}

FetchTask::~FetchTask()
{}

Fetcher::~Fetcher()
{}

Resource::Resource(const std::string &name) : name(name),
    ramMemoryCost(0), gpuMemoryCost(0)
{
    impl = std::shared_ptr<ResourceImpl>(new ResourceImpl(this));
}

Resource::~Resource()
{}

vts::Resource::operator bool() const
{
    return impl->state == ResourceImpl::State::ready;
}

void ResourceImpl::saveToCache(MapImpl *map)
{
    try
    {
        writeLocalFileBuffer(map->convertNameToCache(resource->name),
                       contentData);
    }
    catch(std::runtime_error &)
    {}
}

void ResourceImpl::loadFromCache(MapImpl *map)
{
    code = 0;
    try
    {
        std::string path = map->convertNameToCache(resource->name);
        contentData = readLocalFileBuffer(path);
        code = 200;
        state = ResourceImpl::State::downloaded;
    }
    catch (std::runtime_error &)
    {
        LOG(err3) << "Error reading resource '" + resource->name
                     + "'' from cache file";
    }
}

void ResourceImpl::loadFromInternalMemory()
{
    code = 0;
    try
    {
        contentData = readInternalMemoryBuffer(resource->name);
        code = 200;
        state = ResourceImpl::State::downloaded;
    }
    catch (std::runtime_error &)
    {
        LOG(err3) << "Error reading resource '" + resource->name
                     + "'' from internal memory";
    }
}

ResourceImpl::ResourceImpl(Resource *resource)
    : FetchTask(resource->name),
      resource(resource), state(State::initializing),
      lastAccessTick(0), availTest(nullptr), priority(0)
{}

} // namespace vts
