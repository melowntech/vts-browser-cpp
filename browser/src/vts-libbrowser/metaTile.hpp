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

#ifndef METATILE_HPP_s4h4i476dw3
#define METATILE_HPP_s4h4i476dw3

#include <vts-libs/vts/metatile.hpp>

#include "include/vts-browser/math.hpp"
#include "resource.hpp"

namespace vts
{

class Mapconfig;
class CoordManip;
using TileId = vtslibs::registry::ReferenceFrame::Division::Node::Id;
using Extents2 = math::Extents2;

class BoundMetaTile : public Resource
{
public:
    BoundMetaTile(MapImpl *map, const std::string &name);
    void decode() override;
    FetchTask::ResourceType resourceType() const override;

    uint8 flags[vtslibs::registry::BoundLayer::rasterMetatileWidth
        * vtslibs::registry::BoundLayer::rasterMetatileHeight];
};

class MetaNode
{
public:
    struct Obb
    {
        mat4 rotInv;
        vec3 points[2];
    };

    TileId tileId;
    TileId localId;
    Extents2 extents;
    vec3 aabbPhys[2];
    boost::optional<Obb> obb;
    boost::optional<vec3> surrogatePhys;
    boost::optional<float> surrogateNav;
    vec3 diskNormalPhys;
    vec2 diskHeightsPhys;
    double diskHalfAngle;
    double texelSize;

    MetaNode();
    vec3 cornersPhys(uint32 index) const;
};

Extents2 subExtents(const Extents2 &parentExtents,
    const TileId &parentId, const TileId &targetId);

MetaNode generateMetaNode(const std::shared_ptr<Mapconfig> &m,
    const vtslibs::vts::TileId &id, const vtslibs::vts::MetaNode &meta);
MetaNode generateMetaNode(const std::shared_ptr<Mapconfig> &m,
    std::shared_ptr<CoordManip> cnv,
    const vtslibs::vts::TileId &id, const vtslibs::vts::MetaNode &meta);

class MetaTile : public Resource, public vtslibs::vts::MetaTile
{
public:
    MetaTile(MapImpl *map, const std::string &name);
    void decode() override;
    FetchTask::ResourceType resourceType() const override;
    std::shared_ptr<const MetaNode> getNode(const TileId &tileId);

private:
    std::weak_ptr<Mapconfig> mapconfig;
    std::vector<boost::optional<MetaNode>> metas;
};

} // namespace vts

#endif
