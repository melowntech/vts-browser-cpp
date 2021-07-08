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

#include "../include/vts-browser/cameraDraws.hpp"
#include "renderTasks.hpp"
#include "metaTile.hpp"

#include <boost/container/small_vector.hpp>

namespace vts
{

class MapLayer;
class SurfaceInfo;
class Resource;
class MeshAggregate;
class GeodataTile;

class TraverseNode : private Immovable
{
public:
    // traversal
    boost::container::small_vector<std::unique_ptr<TraverseNode>, 4> childs;
    const MapLayer *const layer = nullptr;
    TraverseNode *const parent = nullptr;
    const TileId id;

    // metadata
    boost::container::small_vector<vtslibs::registry::CreditId, 8> credits;
    boost::container::small_vector<std::shared_ptr<MetaTile>, 1> metaTiles;
    std::shared_ptr<const MetaNode> meta;
    const SurfaceInfo *surface = nullptr;

    uint32 lastAccessTime = 0;
    uint32 lastRenderTime = 0;
    float priority = nan1();

    // renders
    bool determined = false; // draws are fully loaded (may be empty)
    std::vector<std::shared_ptr<Resource>> resources;
    boost::container::small_vector<RenderSurfaceTask, 1> opaque;
    boost::container::small_vector<RenderSurfaceTask, 1> transparent;
    boost::container::small_vector<DrawGeodataTask, 1> geodata;
    boost::container::small_vector<RenderColliderTask, 1> colliders;

    TraverseNode();
    TraverseNode(const MapLayer *layer, TraverseNode *parent, const TileId &id);
    ~TraverseNode();
    void clearAll();
    void clearRenders();
    bool rendersReady() const;
    bool rendersEmpty() const;
};

TraverseNode *findTravById(TraverseNode *trav, const TileId &what);

} // namespace vts

#endif
