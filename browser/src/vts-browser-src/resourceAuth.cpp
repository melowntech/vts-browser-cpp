#include <ctime>
#include <jsoncpp/json.hpp>

#include "include/vts-browser/exceptions.hpp"
#include "map.hpp"

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

AuthConfig::AuthConfig() :
    timeValid(0), timeParsed(0)
{}

void AuthConfig::load()
{
    LOG(info3) << "Parsing authentication json";
    try
    {
        Json::Value root;
        {
            detail::Wrapper w(impl->contentData);
            w >> root;
        }
        int status = root["status"].asInt();
        if (status != 200)
        {
            std::string message = root["statusMessage"].asString();
            LOGTHROW(err3, AuthException) << "Authentication failure ("
                                               << status << ": "
                                               << message << ")";
        }
        uint64 expires = root["expires"].asUInt64();
        uint64 now = root["now"].asUInt64();
        if (expires > now + 60)
            timeValid = expires - now;
        else
            timeValid = 60;
        timeParsed = currentTime();
        Json::Value hostnamesJson = root["hostnames"];
        hostnames.clear();
        for (auto it : hostnamesJson)
            hostnames.insert(it.asString());
        token = root["token"].asString();
        //LOG(info3) << "token: " << token;
    }
    catch (Json::Exception &e)
    {
        LOGTHROW(err3, AuthException) << "Authentication failure ("
                                      << e.what() << ")";
    }
}

void AuthConfig::checkTime()
{
    if (impl->state == FetchTaskImpl::State::ready)
    {
        uint64 t = currentTime();
        if (t + 60 > timeParsed + timeValid)
        {
            // force redownload
            impl->state = FetchTaskImpl::State::initializing;
        }
    }
}

void AuthConfig::authorize(FetchTaskImpl *task)
{
    std::string h = extractUrlHost(task->name);
    if (hostnames.find(h) == hostnames.end())
        return;
    task->queryHeaders["Accept"] = std::string()
            + "token/" + token + ", */*";
}

} // namespace vts

