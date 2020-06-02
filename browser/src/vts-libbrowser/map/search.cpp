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

#include "../include/vts-browser/search.hpp"

#include "../utilities/json.hpp"
#include "../searchTask.hpp"
#include "../coordsManip.hpp"
#include "../fetchTask.hpp"
#include "../mapConfig.hpp"
#include "../map.hpp"

#include <optick.h>

namespace vts
{

namespace
{

std::string generateSearchUrl(MapImpl *impl, const std::string &query,
    const double center[])
{
    std::string url = impl->mapconfig->browserOptions.searchUrl;
    const auto &replace = [&](const std::string &what,
        const std::string with)
    {
        auto s = url.find(what);
        if (s != std::string::npos)
            url.replace(s, what.length(), with);
    };
    {
        std::stringstream ss;
        ss << center[1];
        replace("{lat}", ss.str());
    }
    {
        std::stringstream ss;
        ss << center[0];
        replace("{long}", ss.str());
    }
    replace("{value}", utility::urlEncode(query));
    return url;
}

void searchToNav(MapImpl *map, double point[3])
{
    vec3 p = rawToVec3(point);
    p = map->convertor->searchToNav(p);
    vecToRaw(p, point);
}

// both points are in navigation srs
double distance(MapImpl *map, const double a[3], const double b[3],
                double def = nan1())
{
    for (int i = 0; i < 3; i++)
        if (std::isnan(a[i]) || std::isnan(b[i]))
            return def;
    vec3 va = rawToVec3(a);
    vec3 vb = rawToVec3(b);
    switch (map->mapconfig->srs.get(
                map->mapconfig->referenceFrame.model.navigationSrs).type)
    {
    case vtslibs::registry::Srs::Type::cartesian:
    case vtslibs::registry::Srs::Type::projected:
        return length(vec3(vb - va));
    case vtslibs::registry::Srs::Type::geographic:
        return map->convertor->geoDistance(va, vb);
    }
    return def;
}

double radius(MapImpl *map, const double center[3], const Json::Value &bj)
{
    std::string s = bj.asString();
    if (s.empty())
        return nan1();
    std::stringstream ss(s);
    char c = 0;
    double r[4] = { nan1(), nan1(), nan1(), nan1() };
    ss >> r[0] >> c >> r[1] >> c >> r[2] >> c >> r[3];
    double bbs[4][3] = {
        { r[1], r[0], 0 },
        { r[1], r[2], 0 },
        { r[3], r[0], 0 },
        { r[3], r[2], 0 }
    };
    double radius = 0;
    for (int i = 0; i < 4; i++)
    {
        searchToNav(map, bbs[i]);
        radius = std::max(radius, distance(map, center, bbs[i]));
    }
    return radius;
}

} // namespace

void MapImpl::parseSearchResults(const std::shared_ptr<SearchTask> &task)
{
    assert(!task->done);
    try
    {
        Json::Value root;
        try
        {
            root = stringToJson(task->impl->data.str());
        }
        catch(const std::exception &e)
        {
            LOGTHROW(err2, std::runtime_error)
                    << "Failed to parse search result json, url: <"
                    << task->impl->name << ">, error: <"
                    << e.what() << ">";
        }
        for (const Json::Value &it : root["data"])
        {
            SearchItem t(jsonToString(it));
            searchToNav(this, t.position);
            t.distance = distance(this, task->position, t.position);
            t.radius = radius(this, t.position, it["bounds"]);
            task->results.push_back(std::move(t));
        }
    }
    catch (const std::exception &e)
    {
        LOG(err3) << "Failed to process search results, url: <"
                  << task->impl->name << ">, query: <"
                  << task->query << ">, error: <"
                  << e.what() << ">";
    }
}

SearchTaskImpl::SearchTaskImpl(MapImpl *map, const std::string &name) :
    Resource(map, name),
    validityUrl(map->mapconfig->browserOptions.searchUrl),
    validitySrs(map->mapconfig->browserOptions.searchSrs)
{}

void SearchTaskImpl::decode()
{
    data = std::move(fetch->reply.content);
}

FetchTask::ResourceType SearchTaskImpl::resourceType() const
{
    return FetchTask::ResourceType::Search;
}

SearchItem::SearchItem() :
    position{nan1(), nan1(), nan1()},
    distance(nan1()), radius(nan1())
{}

SearchItem::SearchItem(const std::string &json) :
    SearchItem()
{
    this->json = json;
    Json::Value v = stringToJson(json);
    AJ(id, asString);
    AJ(title, asString);
    AJ(type, asString);
    AJ(region, asString);
    position[0] = v["lon"].asDouble();
    position[1] = v["lat"].asDouble();
    position[2] = 0;
}

SearchTask::SearchTask(const std::string &query, const double point[3]) :
    query(query), position{point[0], point[1], point[2]}, done(false)
{}

void SearchTask::updateDistances(const double point[3])
{
    OPTICK_EVENT();
    if (!impl || !impl->map->mapconfig || !impl->map->mapconfigReady
        || impl->map->mapconfig->browserOptions.searchUrl!=impl->validityUrl
        || impl->map->mapconfig->browserOptions.searchSrs!=impl->validitySrs)
    {
        LOGTHROW(err1, std::runtime_error) << "Search is no longer valid";
    }
    for (auto &it : results)
    {
        it.distance = distance(impl->map, it.position, point);
    }
}

std::shared_ptr<SearchTask> MapImpl::search(const std::string &query,
                                            const double point[3])
{
    OPTICK_EVENT();
    auto t = std::make_shared<SearchTask>(query, point);
    t->impl = getSearchTask(generateSearchUrl(this, query, point));
    t->impl->priority = inf1();
    if (!t->impl->fetch)
        t->impl->fetch = std::make_shared<FetchTaskImpl>(t->impl);
    t->impl->fetch->query.headers["Accept-Language"] = "en-US,en";
    resources.searchTasks.push_back(t);
    return t;
}

void MapImpl::updateSearch()
{
    OPTICK_EVENT();
    auto it = resources.searchTasks.begin();
    while (it != resources.searchTasks.end())
    {
        std::shared_ptr<SearchTask> t = it->lock();
        if (t)
        {
            switch (getResourceValidity(t->impl))
            {
            case Validity::Indeterminate:
                touchResource(t->impl);
                it++;
                continue;
            case Validity::Invalid:
                break;
            case Validity::Valid:
                parseSearchResults(t);
                break;
            }
            t->done = true;
        }
        it = resources.searchTasks.erase(it);
    }
}

} // namespace vts
