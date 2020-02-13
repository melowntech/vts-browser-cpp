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

#ifndef MAPLAYER_HPP_kfd697hgf4t
#define MAPLAYER_HPP_kfd697hgf4t

#include <vts-libs/vts/tsmap.hpp>

#include "renderInfos.hpp"
#include "credits.hpp"

namespace vts
{

class TraverseNode;

class SurfaceInfo
{
public:
    SurfaceInfo() = default;
    SurfaceInfo(const vtslibs::vts::SurfaceCommonConfig &surface,
        const std::string &parentPath);
    SurfaceInfo(const vtslibs::registry::FreeLayer::MeshTiles &surface,
        const std::string &parentPath);
    SurfaceInfo(const vtslibs::registry::FreeLayer::GeodataTiles &surface,
        const std::string &parentPath);
    SurfaceInfo(const vtslibs::registry::FreeLayer::Geodata &surface,
        const std::string &parentPath);

    UrlTemplate urlMeta;
    UrlTemplate urlMesh;
    UrlTemplate urlIntTex;
    UrlTemplate urlGeodata;
    vtslibs::vts::TilesetIdList name;
    vec3f color {0,0,0};
    bool alien = false;
};

class SurfaceStack
{
public:
    void print();
    void colorize();

    void generateVirtual(MapImpl *map,
        const vtslibs::vts::VirtualSurfaceConfig *virtualSurface);
    void generateTileset(MapImpl *map,
        const std::vector<std::string> &vsId,
        const vtslibs::vts::TilesetReferencesList &dataRaw);
    void generateReal(MapImpl *map);
    void generateFree(MapImpl *map, const FreeInfo &freeLayer);

    std::vector<SurfaceInfo> surfaces;
};

class MapLayer : private Immovable
{
public:
    // main surface stack
    MapLayer(MapImpl *map);
    // free layer
    MapLayer(MapImpl *map, const std::string &name,
        const vtslibs::registry::View::FreeLayerParams &params);

    bool prerequisitesCheck();
    bool isGeodata();

    BoundParamInfo::List boundList(
        const SurfaceInfo *surface, sint32 surfaceReference);

    vtslibs::registry::View::Surfaces boundLayerParams;

    std::string freeLayerName;
    boost::optional<FreeInfo> freeLayer;
    boost::optional<vtslibs::registry::View::FreeLayerParams> freeLayerParams;

    SurfaceStack surfaceStack;
    boost::optional<SurfaceStack> tilesetStack;

    std::unique_ptr<TraverseNode> traverseRoot;
    std::unique_ptr<TraverseNode> traverseRoot2;

    MapImpl *const map = nullptr;
    Credits::Scope creditScope;

private:
    bool prerequisitesCheckMainSurfaces();
    bool prerequisitesCheckFreeLayer();
};

} // namespace vts

#endif
