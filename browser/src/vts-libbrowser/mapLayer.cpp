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

#include "../map.hpp"

namespace vts
{

MapLayer::MapLayer(MapImpl *map) : map(map)
{}

bool MapLayer::prerequisitesCheck()
{
    auto *mapConfig = map->mapConfig.get();

    // check for virtual surface
    if (!map->options.debugDisableVirtualSurfaces)
    {
        std::vector<std::string> viewSurfaces;
        viewSurfaces.reserve(mapConfig->view.surfaces.size());
        for (auto &it : mapConfig->view.surfaces)
            viewSurfaces.push_back(it.first);
        std::sort(viewSurfaces.begin(), viewSurfaces.end());
        for (vtslibs::vts::VirtualSurfaceConfig &it
             : mapConfig->virtualSurfaces)
        {
            std::vector<std::string> virtSurfaces(it.id.begin(), it.id.end());
            if (virtSurfaces.size() != viewSurfaces.size())
                continue;
            std::vector<std::string> virtSurfaces2(virtSurfaces);
            std::sort(virtSurfaces2.begin(), virtSurfaces2.end());
            if (!boost::algorithm::equals(viewSurfaces, virtSurfaces2))
                continue;
            auto tilesetMapping = map->getTilesetMapping(
                    convertPath(it.mapping, mapConfig->name));
            if (!testAndThrow(tilesetMapping->state,
                              "Tileset mapping failure."))
                return false;
            surfaceStack.emplace();
            surfaceStack->generateVirtual(map, &it);
            tilesetStack.emplace();
            tilesetStack->generateTileset(map, virtSurfaces,
                                          tilesetMapping->dataRaw);
            break;
        }
    }

    // make real surface stack if no virtual was made
    if (!surfaceStack)
    {
        surfaceStack.emplace();
        surfaceStack->generateReal(map);
    }

    traverseRoot = std::make_shared<TraverseNode>(this, nullptr, NodeInfo(
                    mapConfig->referenceFrame, TileId(), false, *mapConfig));
    traverseRoot->priority = std::numeric_limits<double>::infinity();

    return true;
}

} // namespace vts
