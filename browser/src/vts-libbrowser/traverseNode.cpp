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

#include "map.hpp"

namespace vts
{

namespace
{

uint32 hashf(const NodeInfo &node)
{
    auto id = node.nodeId();
    return std::hash<uint32>()(((uint32)id.x << id.lod) + id.y);
}

} // namespace

TraverseNode::TraverseNode(vts::MapLayer *layer, TraverseNode *parent,
                           const NodeInfo &nodeInfo)
    : layer(layer), parent(parent),
      nodeInfo(nodeInfo),
      hash(hashf(nodeInfo)),
      surface(nullptr),
      lastAccessTime(0), lastRenderTime(0),
      priority(std::numeric_limits<double>::quiet_NaN())
{
    // initialize corners to NAN
    {
        vec3 n;
        for (uint32 i = 0; i < 3; i++)
            n[i] = std::numeric_limits<double>::quiet_NaN();
        for (uint32 i = 0; i < 8; i++)
            cornersPhys[i] = n;
        surrogatePhys = n;
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
    if (lastRenderTime + 5 > layer->map->renderer.tickIndex)
        return;
    opaque.clear();
    transparent.clear();
    geodata.clear();
    colliders.clear();
    touchResource.reset();
}

bool TraverseNode::rendersReady() const
{
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

