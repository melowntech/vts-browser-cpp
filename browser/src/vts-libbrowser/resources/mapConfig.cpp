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

#include <vts-libs/vts/mapconfig-json.hpp>

#include "../mapConfig.hpp"
#include "../map.hpp"
#include "../fetchTask.hpp"
#include "../credits.hpp"
#include "../renderInfos.hpp"

namespace vts
{

Mapconfig::BrowserOptions::BrowserOptions() :
    autorotate(0), searchFilter(true)
{}

Mapconfig::Mapconfig(MapImpl *map, const std::string &name)
    : Resource(map, name)
{
    priority = std::numeric_limits<float>::infinity();
}

void Mapconfig::load()
{
    assert(map->layers.empty());
    LOG(info2) << "Parsing mapconfig <" << name << ">";

    // clear
    *(vtslibs::vts::MapConfig*)this = vtslibs::vts::MapConfig();
    browserOptions = BrowserOptions();
    atmosphereDensityTexture.reset();
    boundInfos.clear();
    freeInfos.clear();

    // load
    {
        detail::BufferStream w(fetch->reply.content);
        vtslibs::vts::loadMapConfig(*this, w, name);
    }

    // search url on earth
    if (isEarth() || map->createOptions.searchUrlFallbackOutsideEarth)
    {
        browserOptions.searchUrl = map->createOptions.searchUrlFallback;
        browserOptions.searchSrs = map->createOptions.searchSrsFallback;
        browserOptions.searchFilter = map->options.searchResultsFiltering;
    }

    // browser options
    auto bo(vtslibs::vts::browserOptions(*this));
    if (bo.isObject())
    {
        if (bo.isMember("rotate") && bo["rotate"].isDouble())
            browserOptions.autorotate = bo["rotate"].asDouble() * 0.1;

        if (map->createOptions.browserOptionsSearchUrls)
        {
            if (bo.isMember("controlSearchUrl"))
                browserOptions.searchUrl
                = bo["controlSearchUrl"].asString();
            if (bo.isMember("controlSearchSrs"))
                browserOptions.searchSrs
                = bo["controlSearchSrs"].asString();
            if (bo.isMember("controlSearchFilter"))
                browserOptions.searchFilter
                = bo["controlSearchFilter"].asBool();
        }
    }
    else
        bo = Json::objectValue;

    if (!bo.isMember("mapFeaturesReduceMode"))
    {
        bo["mapFeaturesReduceMode"] = "scr-count7";
        bo["mapFeaturesReduceParams"][0] = 0.05;
        bo["mapFeaturesReduceParams"][1] = 0.15;
        bo["mapFeaturesReduceParams"][2] = 11;
        bo["mapFeaturesReduceParams"][3] = 1;
        bo["mapFeaturesReduceParams"][4] = 1000;
    }

    browserOptions.value = std::make_shared<Json::Value>(bo);

    if (browserOptions.searchSrs.empty())
        browserOptions.searchUrl = "";

    // store default view
    namedViews[""] = view;
    consolidateView();

    // memory use
    info.ramMemoryCost += sizeof(*this);
}

FetchTask::ResourceType Mapconfig::resourceType() const
{
    return FetchTask::ResourceType::Mapconfig;
}

vtslibs::registry::Srs::Type Mapconfig::navigationSrsType() const
{
    return srs.get(referenceFrame.model.navigationSrs).type;
}

BoundInfo *Mapconfig::getBoundInfo(const std::string &id)
{
    auto it = boundInfos.find(id);
    if (it != boundInfos.end())
        return it->second.get();

    const vtslibs::registry::BoundLayer *bl
            = boundLayers.get(id, std::nothrow);
    if (bl)
    {
        if (bl->external())
        {
            std::string url = convertPath(bl->url, name);
            std::shared_ptr<ExternalBoundLayer> r
                    = map->getExternalBoundLayer(url);
            if (!testAndThrow(r->state, "External bound layer failure."))
                return nullptr;
            boundInfos[bl->id] = std::make_shared<BoundInfo>(*r, url);

            // merge credits
            for (auto &c : r->credits)
                if (c.second)
                    map->credits->merge(*c.second);
        }
        else
        {
            boundInfos[bl->id] = std::make_shared<BoundInfo>(*bl, name);
        }
    }

    return nullptr;
}

FreeInfo *Mapconfig::getFreeInfo(const std::string &id)
{
    auto it = freeInfos.find(id);
    if (it != freeInfos.end())
        return it->second.get();

    const vtslibs::registry::FreeLayer *bl
            = freeLayers.get(id, std::nothrow);
    if (bl)
    {
        if (bl->external())
        {
            std::string url = convertPath(bl->externalUrl(), name);
            std::shared_ptr<ExternalFreeLayer> r
                    = map->getExternalFreeLayer(url);
            if (!testAndThrow(r->state, "External free layer failure."))
                return nullptr;
            freeInfos[bl->id] = std::make_shared<FreeInfo>(*r, url);

            // merge credits
            for (auto &c : r->credits)
                if (c.second)
                    map->credits->merge(*c.second);
        }
        else
        {
            freeInfos[bl->id] = std::make_shared<FreeInfo>(*bl, name);
        }
    }

    return nullptr;
}

vtslibs::vts::SurfaceCommonConfig *Mapconfig::findGlue(
        const vtslibs::vts::Glue::Id &id)
{
    for (auto &it : glues)
    {
        if (it.id == id)
            return &it;
    }
    return nullptr;
}

vtslibs::vts::SurfaceCommonConfig *Mapconfig::findSurface(
        const std::string &id)
{
    for (auto &it : surfaces)
    {
        if (it.id == id)
            return &it;
    }
    return nullptr;
}

void Mapconfig::consolidateView()
{
    // remove invalid surfaces from current view
    std::set<std::string> resSurf;
    for (auto &it : surfaces)
        resSurf.insert(it.id);
    for (auto it = view.surfaces.begin(); it != view.surfaces.end();)
    {
        if (resSurf.find(it->first) == resSurf.end())
        {
            LOG(warn1) << "Removing invalid surface <"
                       << it->first << "> from current view";
            it = view.surfaces.erase(it);
        }
        else
            it++;
    }

    // remove invalid bound layers from surfaces in current view
    std::set<std::string> resBound;
    for (auto &it : boundLayers)
        resBound.insert(it.id);
    for (auto &s : view.surfaces)
    {
        for (auto it = s.second.begin(); it != s.second.end();)
        {
            if (resBound.find(it->id) == resBound.end())
            {
                LOG(warn1) << "Removing invalid bound layer <"
                           << it->id << "> from current view";
                it = s.second.erase(it);
            }
            else
                it++;
        }
    }

    // remove invalid free layers from current view
    // todo

    // remove invalid bound layers from free layers in current view
    // todo
}

} // namespace vts
