#include "resource.hpp"

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

const std::string convertNameToPath(std::string path,
                                           bool preserveSlashes)
{
    path = boost::filesystem::path(path).normalize().string();
    std::string res;
    res.reserve(path.size());
    for (char it : path)
    {
        if ((it >= 'a' && it <= 'z')
         || (it >= 'A' && it <= 'Z')
         || (it >= '0' && it <= '9')
         || (it == '-' || it == '.'))
            res += it;
        else if (preserveSlashes && (it == '/' || it == '\\'))
            res += '/';
        else
            res += '_';
    }
    return res;
}

const std::string convertNameToCache(const std::string &path)
{
    uint32 p = path.find("://");
    std::string a = p == std::string::npos ? path : path.substr(p + 3);
    std::string b = boost::filesystem::path(a).parent_path().string();
    std::string c = a.substr(b.length() + 1);
    return std::string("cache/")
            + convertNameToPath(b, false) + "/"
            + convertNameToPath(c, false);
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

void ResourceImpl::saveToCache()
{
    try
    {
        writeLocalFileBuffer(convertNameToCache(resource->name),
                       contentData);
    }
    catch(std::runtime_error &)
    {}
}

void ResourceImpl::loadFromCache()
{
    code = 0;
    try
    {
        std::string path = convertNameToCache(resource->name);
        contentData = readLocalFileBuffer(path);
        code = 200;
        state = ResourceImpl::State::downloaded;
    }
    catch (std::runtime_error &)
    {
        LOG(err4) << "Error reading resource: " + resource->name
                     + " from cache file";
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
        LOG(err4) << "Error reading resource: " + resource->name
                     + " from internal memory";
    }
}

ResourceImpl::ResourceImpl(Resource *resource)
    : FetchTask(resource->name),
      resource(resource), state(State::initializing),
      lastAccessTick(0), availTest(nullptr)
{}

bool availableInCache(const std::string &name)
{
    std::string path = convertNameToCache(name);
    return boost::filesystem::exists(path);
}

} // namespace vts
