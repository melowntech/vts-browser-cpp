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

void tileWrap(TileId::index_type m, TileId::index_type &x, sint32 d)
{
    x = (x + m + d) % m;
}

uint32 childIndex(const TileId &me, const TileId &child)
{
    assert(child.lod > me.lod);
    TileId t(child);
    while (t.lod > me.lod + 1)
        t = vtslibs::vts::parent(t);
    return vtslibs::vts::child(t);
}

} // namespace

void MapImpl::gridPreloadRequest(TraverseNode *trav)
{
    assert(trav);
    assert(trav->surface);
    if (options.balancedGridLodOffset == (uint32)-1)
        return;

    for (uint32 lodOffset = 0; lodOffset < options.balancedGridLodOffset;
        lodOffset++)
    {
        if (!trav->parent || !trav->parent->surface)
            break;
        trav = trav->parent;
    }

    auto &glr = trav->layer->gridLoadRequests;

    sint32 D = options.balancedGridNeighborsDistance;
    TileId base = trav->nodeInfo.nodeId();
    TileId::index_type m = 1 << base.lod;
    for (sint32 y = -D; y <= D; y++)
    {
        for (sint32 x = -D; x <= D; x++)
        {
            TileId t = base;
            tileWrap(m, t.x, x);
            tileWrap(m, t.y, y);
            glr.push_back(t);
        }
    }
}

void MapImpl::gridPreloadProcess()
{
    statistics.currentGridNodes = 0;
    for (auto &it : layers)
    {
        auto &glr = it->gridLoadRequests;
        std::sort(glr.begin(), glr.end());
        glr.erase(std::unique(glr.begin(), glr.end()), glr.end());
        statistics.currentGridNodes += glr.size();
        gridPreloadProcess(it->traverseRoot.get(), glr);
        glr.clear();
    }
}

void MapImpl::gridPreloadProcess(TraverseNode *trav,
    const std::vector<TileId> &requests)
{
    if (requests.empty())
        return;
    if (!travInit(trav))
        return;

    TileId myId = trav->nodeInfo.nodeId();
    std::vector<TileId> childRequests[4];
    for (auto &cr : childRequests)
        cr.reserve(requests.size());
    for (const TileId &t : requests)
    {
        assert(t.lod >= myId.lod);
        if (t.lod == myId.lod)
        {
            assert(t == myId);
            touchDraws(trav);
            if (trav->surface && trav->rendersEmpty())
                travDetermineDraws(trav);

            // the resources may not be unloaded by other traversals
            trav->lastRenderTime = trav->lastAccessTime;
        }
        else
            childRequests[childIndex(myId, t)].push_back(t);
    }

    for (const auto &c : trav->childs)
        gridPreloadProcess(c.get(), childRequests[
            childIndex(myId, c->nodeInfo.nodeId())]);

    trav->clearRenders();
}

} // namespace vts
