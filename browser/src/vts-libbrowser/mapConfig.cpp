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

#include <utility/uri.hpp>
#include <boost/algorithm/string.hpp>
#include <vts-libs/vts/mapconfig-json.hpp>
#include <ogr_spatialref.h>

#include "map.hpp"

namespace vts
{

MapCelestialBody::MapCelestialBody() : name("undefined"),
    majorRadius(0), minorRadius(0), atmosphereThickness(0),
    atmosphereColorLow{0,0,0,0}, atmosphereColorHigh{0,0,0,0}
{}

MapConfig::SurfaceInfo::SurfaceInfo(const SurfaceCommonConfig &surface,
                         const std::string &parentPath)
    : SurfaceCommonConfig(surface)
{
    urlMeta.parse(MapConfig::convertPath(urls3d->meta, parentPath));
    urlMesh.parse(MapConfig::convertPath(urls3d->mesh, parentPath));
    urlIntTex.parse(MapConfig::convertPath(urls3d->texture, parentPath));
    urlNav.parse(MapConfig::convertPath(urls3d->nav, parentPath));
}

MapConfig::BoundInfo::BoundInfo(const vtslibs::registry::BoundLayer &bl,
                                const std::string &url)
    : BoundLayer(bl)
{
    urlExtTex.parse(MapConfig::convertPath(this->url, url));
    if (metaUrl)
    {
        urlMeta.parse(MapConfig::convertPath(*metaUrl, url));
        urlMask.parse(MapConfig::convertPath(*maskUrl, url));
    }
    if (bl.availability)
    {
	    availability = std::make_shared<
        vtslibs::registry::BoundLayer::Availability>(
            *bl.availability);
    }
}

MapConfig::SurfaceStackItem::SurfaceStackItem() : alien(false)
{}

MapConfig::BrowserOptions::BrowserOptions() :
    autorotate(0), searchFilter(true)
{}

MapConfig::MapConfig(MapImpl *map, const std::string &name)
    : Resource(map, name, FetchTask::ResourceType::MapConfig)
{
    priority = std::numeric_limits<float>::infinity();
}

void MapConfig::load()
{
    clear();
    LOG(info3) << "Parsing map config <" << name << ">";
    detail::Wrapper w(reply.content);
    vtslibs::vts::loadMapConfig(*this, w, name);

    if (isEarth() || !map->createOptions.disableSearchUrlFallbackOutsideEarth)
    {
        browserOptions.searchUrl = map->createOptions.searchUrlFallback;
        browserOptions.searchSrs = map->createOptions.searchSrsFallback;
        browserOptions.searchFilter = map->options.enableSearchResultsFilter;
    }

    auto bo(vtslibs::vts::browserOptions(*this));
    if (bo.isObject())
    {
        Json::Value r = bo["rotate"];
        if (r.isDouble())
            browserOptions.autorotate = r.asDouble() * 0.1;
        if (!map->createOptions.disableBrowserOptionsSearchUrls)
        {
            r = bo["controlSearchUrl"];
            if (r.isString())
                browserOptions.searchUrl = r.asString();
            r = bo["controlSearchSrs"];
            if (r.isString())
                browserOptions.searchSrs = r.asString();
            r = bo["controlSearchFilter"];
            if (r.isBool())
                browserOptions.searchFilter = r.asBool();
        }
    }

    if (browserOptions.searchSrs.empty())
        browserOptions.searchUrl = "";

    namedViews[""] = view;

    // memory use
    info.ramMemoryCost += sizeof(*this);
}

void MapConfig::clear()
{
    *(vtslibs::vts::MapConfig*)this = vtslibs::vts::MapConfig();
    surfaceInfos.clear();
    boundInfos.clear();
    surfaceStack.clear();
    browserOptions = BrowserOptions();
}

const std::string MapConfig::convertPath(const std::string &path,
                                         const std::string &parent)
{
    return utility::Uri(parent).resolve(path).str();
}

vtslibs::registry::Srs::Type MapConfig::navigationSrsType() const
{
    return srs.get(referenceFrame.model.navigationSrs).type;
}

vtslibs::vts::SurfaceCommonConfig *MapConfig::findGlue(
        const vtslibs::vts::Glue::Id &id)
{
    for (auto &&it : glues)
    {
        if (it.id == id)
            return &it;
    }
    return nullptr;
}

vtslibs::vts::SurfaceCommonConfig *MapConfig::findSurface(const std::string &id)
{
    for (auto &&it : surfaces)
    {
        if (it.id == id)
            return &it;
    }
    return nullptr;
}

MapConfig::BoundInfo *MapConfig::getBoundInfo(const std::string &id)
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
            switch (map->getResourceValidity(r))
            {
            case Validity::Valid:
                break;
            case Validity::Indeterminate:
            case Validity::Invalid:
                return nullptr; // todo should behave differently when invalid
            }
            boundInfos[bl->id] = std::make_shared<BoundInfo>(*r, url);

            // merge credits
            for (auto &c : r->credits)
                if (c.second)
                    map->renderer.credits.merge(*c.second);
        }
        else
        {
            boundInfos[bl->id] = std::make_shared<BoundInfo>(*bl, name);
        }
    }
    return nullptr;
}

void MapConfig::printSurfaceStack()
{
    std::ostringstream ss;
    ss << "Surface stack: " << std::endl;
    for (auto &&l : surfaceStack)
        ss << (l.alien ? "* " : "  ")
           << std::setw(100) << std::left << (std::string()
           + "[" + boost::algorithm::join(l.surface->name, ",") + "]")
           << " " << l.color.transpose() << std::endl;
    LOG(info3) << ss.str();
}

void MapConfig::generateSurfaceStack(const vtslibs::vts::VirtualSurfaceConfig
                                     *virtualSurface)
{
    if (virtualSurface)
    {
        LOG(info2) << "Generating (virtual) surface stack for <"
                   << boost::algorithm::join(virtualSurface->id, ",") << ">";
        SurfaceStackItem i;
        i.surface = std::make_shared<SurfaceInfo>(*virtualSurface, name);
        i.color = vec3f(0,0,0);
        surfaceStack.push_back(i);
        return;
    }

    LOG(info2) << "Generating (real) surface stack";

    // prepare initial surface stack
    vtslibs::vts::TileSetGlues::list lst;
    for (auto &&s : view.surfaces)
    {
        vtslibs::vts::TileSetGlues ts(s.first);
        for (auto &&g : glues)
        {
            bool active = g.id.back() == ts.tilesetId;
            for (auto &&it : g.id)
                if (view.surfaces.find(it) == view.surfaces.end())
                    active = false;
            if (active)
                ts.glues.push_back(vtslibs::vts::Glue(g.id));
        }
        lst.push_back(ts);
    }

    // sort surfaces by their order in mapConfig
    std::unordered_map<std::string, uint32> order;
    uint32 i = 0;
    for (auto &&it : surfaces)
        order[it.id] = i++;
    std::sort(lst.begin(), lst.end(), [order](
              vtslibs::vts::TileSetGlues &a,
              vtslibs::vts::TileSetGlues &b) mutable {
        assert(order.find(a.tilesetId) != order.end());
        assert(order.find(b.tilesetId) != order.end());
        return order[a.tilesetId] < order[b.tilesetId];
    });

    // sort glues on surfaces
    lst = vtslibs::vts::glueOrder(lst);
    std::reverse(lst.begin(), lst.end());

    // generate proper surface stack
    assert(surfaceStack.empty());
    for (auto &&ts : lst)
    {
        for (auto &&g : ts.glues)
        {
            SurfaceStackItem i;
            i.surface = std::shared_ptr<SurfaceInfo> (new SurfaceInfo(
                    *findGlue(g.id), name));
            i.surface->name = g.id;
            surfaceStack.push_back(i);
        }
        SurfaceStackItem i;
        i.surface = std::make_shared<SurfaceInfo>(*findSurface(ts.tilesetId),
                                                  name);
        i.surface->name = { ts.tilesetId };
        surfaceStack.push_back(i);
    }

    // generate alien surface stack positions
    auto copy(surfaceStack);
    for (auto &&it : copy)
    {
        if (it.surface->name.size() > 1)
        {
            auto n2 = it.surface->name;
            n2.pop_back();
            std::string n = boost::join(n2, "|");
            auto jt = surfaceStack.begin(), et = surfaceStack.end();
            while (jt != et && boost::join(jt->surface->name, "|") != n)
                jt++;
            if (jt != et)
            {
                SurfaceStackItem i(it);
                i.alien = true;
                surfaceStack.insert(jt, i);
            }
        }
    }

    colorizeSurfaceStack(surfaceStack);
}

void MapConfig::colorizeSurfaceStack(std::vector<MapConfig::SurfaceStackItem>
                                     &ss)
{
    for (auto it = ss.begin(),
         et = ss.end(); it != et; it++)
        it->color = convertHsvToRgb(vec3f((it - ss.begin())
                                          / (float)ss.size(), 1, 1));
}

void MapConfig::consolidateView()
{
    // remove invalid surfaces from current view
    std::set<std::string> resSurf;
    for (auto &&it : surfaces)
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
    for (auto &&it : boundLayers)
        resBound.insert(it.id);
    for (auto &&s : view.surfaces)
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

bool MapConfig::isEarth() const
{
    auto n = srs(referenceFrame.model.physicalSrs);
    auto r = n.srsDef.reference();
    auto a = r.GetSemiMajor();
    return std::abs(a - 6378137) < 50000;
}

void MapConfig::initializeCelestialBody() const
{
    map->body = MapCelestialBody();
    auto n = srs(referenceFrame.model.physicalSrs);
    auto r = n.srsDef.reference();
    map->body.majorRadius = r.GetSemiMajor();
    map->body.minorRadius = r.GetSemiMinor();
    if (isEarth())
    {
        map->body.name = "Earth";
        map->body.atmosphereThickness = 50000;
        static auto lowColor = { 216.0/255.0, 232.0/255.0, 243.0/255.0, 1.0 };
        static auto highColor = { 72.0/255.0, 154.0/255.0, 255.0/255.0, 1.0 };
        std::copy(lowColor.begin(), lowColor.end(),
                  map->body.atmosphereColorLow);
        std::copy(highColor.begin(), highColor.end(),
                  map->body.atmosphereColorHigh);
    }
    if (std::abs(map->body.majorRadius - 3396200) < 30000)
    {
        map->body.name = "Mars";
        map->body.atmosphereThickness = 30000;
        static auto lowColor = { 0.77, 0.48, 0.27, 0.5 };
        static auto highColor = { 0.58, 0.65, 0.77, 0.5 };
        std::copy(lowColor.begin(), lowColor.end(),
                  map->body.atmosphereColorLow);
        std::copy(highColor.begin(), highColor.end(),
                  map->body.atmosphereColorHigh);
    }
    else
    {
        map->body.name = "unknown";
    }
}

} // namespace vts
