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

#include "camera.hpp"

namespace vts
{

TraverseNode::TraverseNode(vts::MapLayer *layer, TraverseNode *parent,
                           const NodeInfo &nodeInfo)
    : layer(layer), parent(parent),
      nodeInfo(nodeInfo),
      hash(std::hash<vts::TileId>()(nodeInfo.nodeId())),
      surface(nullptr),
      lastAccessTime(0), lastRenderTime(0),
      priority(nan1())
{
    // initialize corners to NAN
    {
        for (uint32 i = 0; i < 8; i++)
            cornersPhys[i] = nan3();
        surrogatePhys = nan3();
    }
    // initialize disk to NAN
    {
        diskNormalPhys = nan3();
        diskHeightsPhys = nan2();
        diskHalfAngle = nan1();
    }
    // initialize aabb to universe
    {
        double di = std::numeric_limits<double>::infinity();
        vec3 vi(di, di, di);
        aabbPhys[0] = -vi;
        aabbPhys[1] = vi;
    }
}

TraverseNode::~TraverseNode()
{}

void TraverseNode::clearAll()
{
    childs.clear();
    metaTiles.clear();
    meta.reset();
    obb.reset();
    surrogatePhys.reset();
    surrogateNav.reset();
    surface = nullptr;
    credits.clear();
    clearRenders();
}

void TraverseNode::clearRenders()
{
    opaque.clear();
    transparent.clear();
    geodata.clear();
    colliders.clear();
    touchResource.reset();
}

bool TraverseNode::rendersReady() const
{
    assert(!rendersEmpty());
    for (auto &it : opaque)
        if (!it.ready())
            return false;
    for (auto &it : transparent)
        if (!it.ready())
            return false;
    for (auto &it : geodata)
        if (!it.ready())
            return false;
    for (auto &it : colliders)
        if (!it.ready())
            return false;
    return true;
}

bool TraverseNode::rendersEmpty() const
{
    return opaque.empty() && transparent.empty()
        && geodata.empty() && colliders.empty();
}

} // namespace vts

