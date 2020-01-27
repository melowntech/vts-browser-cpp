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

#ifndef TRAVERSENODE_HPP_sgh44f
#define TRAVERSENODE_HPP_sgh44f

#include <vts-libs/vts/nodeinfo.hpp>
#include <vts-libs/vts/metatile.hpp>

#include "include/vts-browser/math.hpp"
#include "utilities/array.hpp"

namespace vts
{

using vtslibs::vts::NodeInfo;
using TileId = vtslibs::registry::ReferenceFrame::Division::Node::Id;

class MapLayer;
class MetaTile;
class SurfaceInfo;
class Resource;
class RenderSurfaceTask;
class RenderColliderTask;
class MeshAggregate;
class GeodataTile;

class TraverseNode : private Immovable
{
public:
    struct Obb
    {
        mat4 rotInv;
        vec3 points[2];
    };

    // traversal
    Array<std::unique_ptr<TraverseNode>, 4> childs;
    MapLayer *const layer;
    TraverseNode *const parent;
    const NodeInfo nodeInfo;
    const uint32 hash;

    // metadata
    std::vector<vtslibs::registry::CreditId> credits;
    std::vector<std::shared_ptr<MetaTile>> metaTiles;
    boost::optional<vtslibs::vts::MetaNode> meta;
    boost::optional<Obb> obb;
    vec3 cornersPhys[8];
    vec3 aabbPhys[2];
    boost::optional<vec3> surrogatePhys;
    boost::optional<float> surrogateNav;
    vec3 diskNormalPhys;
    vec2 diskHeightsPhys;
    double diskHalfAngle;
    double texelSize;
    const SurfaceInfo *surface;

    uint32 lastAccessTime;
    uint32 lastRenderTime;
    float priority;

    // renders
    bool determined; // draws are fully loaded (draws may be empty)
    std::shared_ptr<MeshAggregate> meshAgg;
    std::shared_ptr<GeodataTile> geodataAgg;
    std::vector<RenderSurfaceTask> opaque;
    std::vector<RenderSurfaceTask> transparent;
    std::vector<RenderColliderTask> colliders;

    TraverseNode(MapLayer *layer, TraverseNode *parent,
        const NodeInfo &nodeInfo);
    ~TraverseNode();
    void clearAll();
    void clearRenders();
    bool rendersReady() const;
    bool rendersEmpty() const;
    TileId id() const { return nodeInfo.nodeId(); }
};

TraverseNode *findTravById(TraverseNode *trav, const TileId &what);

} // namespace vts

#endif
