#include <ctime>
#include "jsoncpp/json.hpp"

#include "resource.hpp"
#include "auth.hpp"

namespace vts
{

namespace
{

const std::string extractUrlHost(const std::string &url)
{
    auto a = url.find("://");
    if (a == std::string::npos)
        a = 0;
    else
        a += 3;
    
    auto b = url.find("/", a);
    if (b == std::string::npos)
        b = url.length();
    
    return url.substr(a, b - a);
}

uint64 currentTime()
{
    std::time_t t = std::time(nullptr);
    return t;
}

} // namespace

AuthJson::AuthJson(const std::string &name) :
    Resource(name, false), timeValid(0), timeParsed(0)
{}

void AuthJson::load(class MapImpl *)
{
    LOG(info3) << "Parsing authentication json";
    Json::Value root;
    {
        detail::Wrapper w(impl->contentData);
        w >> root;
    }
    uint64 expires = root["expires"].asUInt64();
    uint64 now = root["now"].asUInt64();
    timeValid = expires - now;
    timeParsed = currentTime();
    Json::Value hostnamesJson = root["hostnames"];
    hostnames.clear();
    for (auto it : hostnamesJson)
        hostnames.insert(it.asString());
    token = root["token"].asString();
    //LOG(info3) << "token: " << token;
}

void AuthJson::checkTime()
{
    if (impl->state == ResourceImpl::State::ready)
    {
        uint64 t = currentTime();
        if (t + 60 > timeParsed + timeValid)
        {
            // force redownload
            impl->state = ResourceImpl::State::initializing;
        }
    }
}

void AuthJson::authorize(std::shared_ptr<Resource> resource)
{
    std::string h = extractUrlHost(resource->impl->name);
    if (hostnames.find(h) == hostnames.end())
        return;
    resource->impl->queryHeaders["Accept"] = std::string()
            + "token/" + token + ", */*";
}

} // namespace vts

