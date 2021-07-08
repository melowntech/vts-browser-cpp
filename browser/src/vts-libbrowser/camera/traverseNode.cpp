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

#include "../camera.hpp"
#include "../traverseNode.hpp"
#include "../renderTasks.hpp"
#include "../hashTileId.hpp"
#include "../geodata.hpp"

namespace vts
{

TraverseNode::TraverseNode()
{}

TraverseNode::TraverseNode(const MapLayer *layer, TraverseNode *parent,
    const TileId &id) : layer(layer), parent(parent), id(id)
{}

TraverseNode::~TraverseNode()
{}

void TraverseNode::clearAll()
{
    childs.clear();
    metaTiles.clear();
    meta.reset();
    surface = nullptr;
    credits.clear();
    clearRenders();
}

void TraverseNode::clearRenders()
{
    resources.clear();
    opaque.clear();
    transparent.clear();
    geodata.clear();
    colliders.clear();
    determined = false;
}

bool TraverseNode::rendersReady() const
{
    assert(determined);
    for (auto &it : opaque)
        if (!it.ready())
            return false;
    for (auto &it : transparent)
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

