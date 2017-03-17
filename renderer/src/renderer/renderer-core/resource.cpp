#include "resource.h"

namespace melown
{

Buffer readLocalFileBuffer(const std::string &path)
{
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        throw std::runtime_error("failed to read file");
    try
    {
        fseek(f, 0, SEEK_END);
        Buffer b(ftell(f));
        fseek(f, 0, SEEK_SET);
        if (fread(b.data(), b.size(), 1, f) != 1)
            throw std::runtime_error("failed to read file");
        fclose(f);
        return b;
    }
    catch (...)
    {
        fclose(f);
        throw;
    }
}

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

melown::Resource::operator bool() const
{
    return impl->state == ResourceImpl::State::ready;
}

ResourceImpl::DownloadTask::DownloadTask(ResourceImpl *resource)
    : FetchTask(resource->resource->name), resource(resource)
{}

void ResourceImpl::DownloadTask::saveToCache()
{
    try
    {
        writeLocalFileBuffer(convertNameToCache(resource->resource->name),
                       contentData);
    }
    catch(std::runtime_error &)
    {}
}

void ResourceImpl::DownloadTask::loadFromCache()
{
    code = 0;
    try
    {
        std::string path = convertNameToCache(resource->resource->name);
        contentData = readLocalFileBuffer(path);
        code = 200;
    }
    catch (std::runtime_error &)
    {}
}

void ResourceImpl::DownloadTask::readLocalFile()
{
    code = 0;
    try
    {
        contentData = readLocalFileBuffer(url);
        code = 200;
    }
    catch (std::runtime_error &)
    {}
}

ResourceImpl::ResourceImpl(Resource *resource)
    : resource(resource), state(State::initializing),
      lastAccessTick(0), availTest(nullptr)
{}

bool availableInCache(const std::string &name)
{
    std::string path = convertNameToCache(name);
    return boost::filesystem::exists(path);
}

} // namespace melown
