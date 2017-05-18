#include "include/vts-browser/fetcher.hpp"
#include "map.hpp"

namespace vts
{

FetchTask::FetchTask(const std::string &name) : queryUrl(name),
    replyCode(0), redirectionsCount(0), name(name)
{}

FetchTask::~FetchTask()
{}

void FetchTask::saveToCache(MapImpl *map)
{
    try
    {
        writeLocalFileBuffer(map->convertNameToCache(name), contentData);
    }
    catch (...)
    {
		// do nothing
	}
}

bool FetchTask::loadFromCache(MapImpl *map)
{
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

void FetchTask::loadFromInternalMemory()
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

const std::string MapImpl::convertNameToCache(const std::string &path)
{
    auto p = path.find("://");
    std::string a = p == std::string::npos ? path : path.substr(p + 3);
    std::string b = boost::filesystem::path(a).parent_path().string();
    std::string c = a.substr(b.length() + 1);
    if (b.empty() || c.empty())
        LOGTHROW(err2, std::runtime_error)
                << "Cannot convert path '" << path
                << "' into a cache path";
    return resources.cachePath
            + convertNameToPath(b, false) + "/"
            + convertNameToPath(c, false);
}

const std::string MapImpl::convertNameToPath(std::string path,
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

} // namespace vts
