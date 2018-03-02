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

#include <boost/algorithm/string.hpp>

#include "../map.hpp"

namespace vts
{

SurfaceStackItem::SurfaceStackItem() : alien(false)
{}

void SurfaceStack::print()
{
    std::ostringstream ss;
    ss << "Surface stack: " << std::endl;
    for (auto &l : surfaces)
        ss << (l.alien ? "* " : "  ")
           << std::setw(100) << std::left << (std::string()
           + "[" + boost::algorithm::join(l.surface->name, ",") + "]")
           << " " << l.color.transpose() << std::endl;
    LOG(info3) << ss.str();
}

void SurfaceStack::colorize()
{
    auto &ss = surfaces;
    for (auto it = ss.begin(),
         et = ss.end(); it != et; it++)
        it->color = convertHsvToRgb(vec3f((it - ss.begin())
                                          / (float)ss.size(), 1, 1));
}

void SurfaceStack::generateVirtual(MapImpl *map,
        const vtslibs::vts::VirtualSurfaceConfig *virtualSurface)
{
    assert(surfaces.empty());
    LOG(info2) << "Generating (virtual) surface stack for <"
               << boost::algorithm::join(virtualSurface->id, ",") << ">";
    SurfaceStackItem i;
    i.surface = std::make_shared<SurfaceInfo>(*virtualSurface,
                                              map->mapConfig->name);
    i.color = vec3f(0,0,0);
    surfaces.push_back(i);
}

void SurfaceStack::generateTileset(MapImpl *map,
        const std::vector<std::string> &vsId,
        const vtslibs::vts::TilesetReferencesList &dataRaw)
{
    auto *mapConfig = map->mapConfig.get();

    assert(surfaces.empty());
    surfaces.reserve(dataRaw.size() + 1);
    // the sourceReference in metanodes is one-based
    surfaces.push_back(SurfaceStackItem());
    for (auto &it : dataRaw)
    {
        if (it.size() == 1)
        { // surface
            SurfaceStackItem i;
            i.surface = std::make_shared<SurfaceInfo>(
                    *mapConfig->findSurface(vsId[it[0]]),
                    mapConfig->name);
            i.surface->name.push_back(vsId[it[0]]);
            surfaces.push_back(i);
        }
        else
        { // glue
            std::vector<std::string> id;
            id.reserve(it.size());
            for (auto &it2 : it)
                id.push_back(vsId[it2]);
            SurfaceStackItem i;
            i.surface = std::make_shared<SurfaceInfo>(
                    *mapConfig->findGlue(id),
                    mapConfig->name);
            i.surface->name = id;
            surfaces.push_back(i);
        }
    }

    colorize();
}

void SurfaceStack::generateReal(MapImpl *map)
{
    LOG(info2) << "Generating (real) surface stack";

    // prepare initial surface stack
    vtslibs::vts::TileSetGlues::list lst;
    for (auto &s : map->mapConfig->view.surfaces)
    {
        vtslibs::vts::TileSetGlues ts(s.first);
        for (auto &g : map->mapConfig->glues)
        {
            bool active = g.id.back() == ts.tilesetId;
            for (auto &it : g.id)
                if (map->mapConfig->view.surfaces.find(it)
                        == map->mapConfig->view.surfaces.end())
                    active = false;
            if (active)
                ts.glues.push_back(vtslibs::vts::Glue(g.id));
        }
        lst.push_back(ts);
    }

    // sort surfaces by their order in mapConfig
    std::unordered_map<std::string, uint32> order;
    uint32 i = 0;
    for (auto &it : map->mapConfig->surfaces)
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
    assert(surfaces.empty());
    for (auto &ts : lst)
    {
        for (auto &g : ts.glues)
        {
            SurfaceStackItem i;
            i.surface = std::make_shared<SurfaceInfo>(
                    *map->mapConfig->findGlue(g.id),
                    map->mapConfig->name);
            i.surface->name = g.id;
            surfaces.push_back(i);
        }
        SurfaceStackItem i;
        i.surface = std::make_shared<SurfaceInfo>(
                    *map->mapConfig->findSurface(ts.tilesetId),
                    map->mapConfig->name);
        i.surface->name = { ts.tilesetId };
        surfaces.push_back(i);
    }

    // generate alien surface stack positions
    auto copy(surfaces);
    for (auto &it : copy)
    {
        if (it.surface->name.size() > 1)
        {
            auto n2 = it.surface->name;
            n2.pop_back();
            std::string n = boost::join(n2, "|");
            auto jt = surfaces.begin(), et = surfaces.end();
            while (jt != et && boost::join(jt->surface->name, "|") != n)
                jt++;
            if (jt != et)
            {
                SurfaceStackItem i(it);
                i.alien = true;
                surfaces.insert(jt, i);
            }
        }
    }

    colorize();
}

SurfaceInfo::SurfaceInfo(const vtslibs::vts::SurfaceCommonConfig &surface,
                         const std::string &parentPath)
{
    urlMeta.parse(convertPath(surface.urls3d->meta, parentPath));
    urlMesh.parse(convertPath(surface.urls3d->mesh, parentPath));
    urlIntTex.parse(convertPath(surface.urls3d->texture, parentPath));
}

} // namespace vts
