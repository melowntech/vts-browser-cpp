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

#include <ctime>
#include <jsoncpp/json.hpp>
#include <dbglog/dbglog.hpp>

#include "../include/vts-browser/exceptions.hpp"

#include "../authConfig.hpp"
#include "../fetchTask.hpp"

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

AuthConfig::AuthConfig(MapImpl *map, const std::string &name) :
    Resource(map, name),
    timeValid(0), timeParsed(0)
{
    priority = std::numeric_limits<float>::infinity();

    static const std::string tokenPrefix = "token:";
    if (name.substr(0, tokenPrefix.size()) == tokenPrefix)
    {
        token = name.substr(tokenPrefix.size());
        timeParsed = currentTime();
        timeValid = (uint64)60 * 60 * 24 * 365 * 100;
        state = Resource::State::ready;
    }
}

void AuthConfig::load()
{
    LOG(info2) << "Parsing authentication json";
    try
    {
        Json::Value root;
        {
            detail::BufferStream w(fetch->reply.content);
            w >> root;
        }
        int status = root["status"].asInt();
        if (status != 200)
        {
            std::string message = root["statusMessage"].asString();
            LOGTHROW(err3, AuthException) << "Authentication failure ("
                                        << status << ": " << message << ")";
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
        info.ramMemoryCost += sizeof(*this);
    }
    catch (Json::Exception &e)
    {
        LOGTHROW(err3, AuthException) << "Authentication failure ("
                                      << e.what() << ")";
    }
}

FetchTask::ResourceType AuthConfig::resourceType() const
{
    return FetchTask::ResourceType::AuthConfig;
}

void AuthConfig::checkTime()
{
    if (state == Resource::State::ready)
    {
        uint64 t = currentTime();
        if (t + 60 > timeParsed + timeValid)
        {
            // force redownload
            state = Resource::State::initializing;
        }
    }
}

void AuthConfig::authorize(const std::shared_ptr<Resource> &task)
{
    if (!hostnames.empty())
    {
        std::string h = extractUrlHost(task->name);
        if (hostnames.find(h) == hostnames.end())
            return;
    }
    task->fetch->query.headers["Accept"] = std::string()
            + "token/" + token + ", */*";
}

} // namespace vts

