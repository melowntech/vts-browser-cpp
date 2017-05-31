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

#include "resources.hpp"

namespace vts
{

MapConfig::SurfaceInfo::SurfaceInfo(const SurfaceCommonConfig &surface,
                         const std::string &parentPath)
    : SurfaceCommonConfig(surface)
{
    urlMeta.parse(MapConfig::convertPath(urls3d->meta, parentPath));
    urlMesh.parse(MapConfig::convertPath(urls3d->mesh, parentPath));
    urlIntTex.parse(MapConfig::convertPath(urls3d->texture, parentPath));
    urlNav.parse(MapConfig::convertPath(urls3d->nav, parentPath));
}

MapConfig::BoundInfo::BoundInfo(const BoundLayer &layer) : BoundLayer(layer)
{
    urlExtTex.parse(url);
    if (metaUrl)
    {
        urlMeta.parse(*metaUrl);
        urlMask.parse(*maskUrl);
    }
}

MapConfig::SurfaceStackItem::SurfaceStackItem() : alien(false)
{}

MapConfig::BrowserOptions::BrowserOptions() :
    autorotate(0)
{}

MapConfig::MapConfig()
{}

void MapConfig::load()
{
    clear();
    LOG(info3) << "Parsing map config '" << fetch->name << "'";
    detail::Wrapper w(fetch->contentData);
    vtslibs::vts::loadMapConfig(*this, w, fetch->name);
    
    auto bo(vtslibs::vts::browserOptions(*this));
    if (bo.isObject())
    {
        Json::Value r = bo["rotate"];
        if (r.isDouble())
            browserOptions.autorotate = r.asDouble() * 0.1;
    }
    
    convertor = std::shared_ptr<CsConvertor>(CsConvertor::create(
                  referenceFrame.model.physicalSrs,
                  referenceFrame.model.navigationSrs,
                  referenceFrame.model.publicSrs,
                  *this
                  ));
}

void MapConfig::clear()
{
    *(vtslibs::vts::MapConfig*)this = vtslibs::vts::MapConfig();
    surfaceInfos.clear();
    boundInfos.clear();
    surfaceStack.clear();
    convertor.reset();
    browserOptions.autorotate = 0;
}

const std::string MapConfig::convertPath(const std::string &path,
                                         const std::string &parent)
{
    return utility::Uri(parent).resolve(path).str();
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
    if (it == boundInfos.end())
        return nullptr;
    return it->second.get();
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

void MapConfig::generateSurfaceStack()
{
    // remove invalid surfaces from current view
    std::set<std::string> resSurf;
    for (auto &&it : surfaces)
        resSurf.insert(it.id);
    for (auto it = view.surfaces.begin(); it != view.surfaces.end();)
    {
        if (resSurf.find(it->first) == resSurf.end())
            it = view.surfaces.erase(it);
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
                it = s.second.erase(it);
            else
                it++;
        }
    }
    
    // remove invalid free layers from current view
    // todo
    
    // remove invalid bound layers from free layers in current view
    // todo
    
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
    surfaceStack.clear();
    for (auto &&ts : lst)
    {
        for (auto &&g : ts.glues)
        {
            SurfaceStackItem i;
            i.surface = std::shared_ptr<SurfaceInfo> (new SurfaceInfo(
                    *findGlue(g.id), fetch->name));
            i.surface->name = g.id;
            surfaceStack.push_back(i);
        }
        SurfaceStackItem i;
        i.surface = std::shared_ptr<SurfaceInfo> (new SurfaceInfo(
                    *findSurface(ts.tilesetId), fetch->name));
        i.surface->name = { ts.tilesetId };
        surfaceStack.push_back(i);
    }
    
    // colorize proper surface stack
    for (auto it = surfaceStack.begin(),
         et = surfaceStack.end(); it != et; it++)
        it->color = convertHsvToRgb(vec3f((it - surfaceStack.begin())
                                          / (float)surfaceStack.size(), 1, 1));
    
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
    
    // colorize alien positions
    for (auto it = surfaceStack.begin(),
         et = surfaceStack.end(); it != et; it++)
    {
        if (it->alien)
        {
            vec3f c = convertRgbToHsv(it->color);
            c(2) *= 0.5;
            it->color = convertHsvToRgb(c);
        }
    }
    
    // memory use
    info.ramMemoryCost += sizeof(*this);
}

void ExternalBoundLayer::load()
{
    detail::Wrapper w(fetch->contentData);
    *(vtslibs::registry::BoundLayer*)this
            = vtslibs::registry::loadBoundLayer(w, fetch->name);
}

} // namespace vts
