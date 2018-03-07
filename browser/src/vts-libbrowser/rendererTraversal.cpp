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

vec3 lowerUpperCombine(uint32 i)
{
    vec3 res;
    res(0) = (i >> 0) % 2;
    res(1) = (i >> 1) % 2;
    res(2) = (i >> 2) % 2;
    return res;
}

} // namespace

double MapImpl::travDistance(TraverseNode *trav, const vec3 pointPhys)
{
    if (!vtslibs::vts::empty(trav->meta->geomExtents)
            && !trav->nodeInfo.srs().empty())
    {
        // todo periodicity
        vec2 fl = vecFromUblas<vec2>(trav->nodeInfo.extents().ll);
        vec2 fu = vecFromUblas<vec2>(trav->nodeInfo.extents().ur);
        vec3 el = vec2to3(fl, trav->meta->geomExtents.z.min);
        vec3 eu = vec2to3(fu, trav->meta->geomExtents.z.max);
        vec3 p = convertor->convert(pointPhys,
            Srs::Physical, trav->nodeInfo.node());
        return aabbPointDist(p, el, eu);
    }
    return aabbPointDist(pointPhys, trav->meta->aabbPhys[0],
            trav->meta->aabbPhys[1]);
}

float MapImpl::computeResourcePriority(TraverseNode *trav)
{
    if (options.traverseMode == TraverseMode::Hierarchical)
        return 1.f / trav->nodeInfo.distanceFromRoot();
    if ((trav->hash + renderer.tickIndex) % 4 == 0) // skip expensive function
        return (float)(1e6 / (travDistance(trav, renderer.focusPosPhys) + 1));
    return trav->priority;
}

std::shared_ptr<Resource> MapImpl::travInternalTexture(TraverseNode *trav,
                                                       uint32 subMeshIndex)
{
    UrlTemplate::Vars vars(trav->nodeInfo.nodeId(),
            vtslibs::vts::local(trav->nodeInfo), subMeshIndex);
    std::shared_ptr<Resource> res = getTexture(
                trav->surface->surface->urlIntTex(vars));
    res->updatePriority(trav->priority);
    return res;
}

bool MapImpl::travDetermineMeta(TraverseNode *trav)
{
    assert(!trav->meta);
    assert(trav->childs.empty());
    assert(trav->rendersEmpty());
    assert(!trav->parent || trav->parent->meta);

    // statistics
    statistics.currentNodeMetaUpdates++;

    const TileId nodeId = trav->nodeInfo.nodeId();

    // find all metatiles
    std::vector<std::shared_ptr<MetaTile>> metaTiles;
    metaTiles.resize(trav->layer->surfaceStack->surfaces.size());
    const UrlTemplate::Vars tileIdVars(roundId(nodeId));
    bool determined = true;
    for (uint32 i = 0, e = metaTiles.size(); i != e; i++)
    {
        if (trav->parent)
        {
            const std::shared_ptr<MetaTile> &p
                    = trav->parent->meta->metaTiles[i];
            if (!p)
                continue;
            TileId pid = vtslibs::vts::parent(nodeId);
            uint32 idx = (nodeId.x % 2) + (nodeId.y % 2) * 2;
            const vtslibs::vts::MetaNode &node = p->get(pid);
            if ((node.flags()
                 & (vtslibs::vts::MetaNode::Flag::ulChild << idx)) == 0)
                continue;
        }
        auto m = getMetaTile(trav->layer->surfaceStack->surfaces[i].surface
                             ->urlMeta(tileIdVars));
        m->updatePriority(trav->priority);
        switch (getResourceValidity(m))
        {
        case Validity::Indeterminate:
            determined = false;
            // no break here
        case Validity::Invalid:
            continue;
        case Validity::Valid:
            break;
        }
        metaTiles[i] = m;
    }
    if (!determined)
        return false;

    // find topmost nonempty surface
    SurfaceStackItem *topmost = nullptr;
    const vtslibs::vts::MetaNode *node = nullptr;
    bool childsAvailable[4] = {false, false, false, false};
    for (uint32 i = 0, e = metaTiles.size(); i != e; i++)
    {
        if (!metaTiles[i])
            continue;
        const vtslibs::vts::MetaNode &n = metaTiles[i]->get(nodeId);
        for (uint32 i = 0; i < 4; i++)
            childsAvailable[i] = childsAvailable[i]
                    || (n.childFlags()
                        & (vtslibs::vts::MetaNode::Flag::ulChild << i));
        if (topmost || n.alien()
                != trav->layer->surfaceStack->surfaces[i].alien)
            continue;
        if (n.geometry())
        {
            node = &n;
            if (trav->layer->tilesetStack)
            {
                assert(n.sourceReference > 0 && n.sourceReference
                       <= trav->layer->tilesetStack->surfaces.size());
                topmost = &trav->layer->tilesetStack
                        ->surfaces[n.sourceReference];
            }
            else
                topmost = &trav->layer->surfaceStack->surfaces[i];
        }
        if (!node)
            node = &n;
    }
    if (!node)
        return false; // all surfaces failed to download, what can i do?

    trav->meta = *node;
    trav->meta->metaTiles.swap(metaTiles);

    // corners
    if (!vtslibs::vts::empty(node->geomExtents)
            && !trav->nodeInfo.srs().empty())
    {
        vec2 fl = vecFromUblas<vec2>(trav->nodeInfo.extents().ll);
        vec2 fu = vecFromUblas<vec2>(trav->nodeInfo.extents().ur);
        vec3 el = vec2to3(fl, node->geomExtents.z.min);
        vec3 eu = vec2to3(fu, node->geomExtents.z.max);
        vec3 ed = eu - el;
        vec3 *corners = trav->meta->cornersPhys;
        for (uint32 i = 0; i < 8; i++)
        {
            vec3 f = lowerUpperCombine(i).cwiseProduct(ed) + el;
            f = convertor->convert(f, trav->nodeInfo.node(), Srs::Physical);
            corners[i] = f;
        }

        // obb
        if (trav->nodeInfo.distanceFromRoot() > 4)
        {
            vec3 center = vec3(0,0,0);
            for (uint32 i = 0; i < 8; i++)
                center += corners[i];
            center /= 8;

            vec3 f = corners[4] - corners[0];
            vec3 u = corners[2] - corners[0];
            mat4 t = lookAt(center, center + f, u);

            TraverseNode::Obb obb;
            obb.rotInv = t.inverse();
            double di = std::numeric_limits<double>::infinity();
            vec3 vi(di, di, di);
            obb.points[0] = vi;
            obb.points[1] = -vi;

            for (uint32 i = 0; i < 8; i++)
            {
                vec3 p = vec4to3(t * vec3to4(corners[i], 1), false);
                obb.points[0] = min(obb.points[0], p);
                obb.points[1] = max(obb.points[1], p);
            }

            trav->meta->obb = obb;
        }
    }
    else if (node->extents.ll != node->extents.ur)
    {
        vec3 fl = vecFromUblas<vec3>(node->extents.ll);
        vec3 fu = vecFromUblas<vec3>(node->extents.ur);
        vec3 fd = fu - fl;
        vec3 el = vecFromUblas<vec3>
                (mapConfig->referenceFrame.division.extents.ll);
        vec3 eu = vecFromUblas<vec3>
                (mapConfig->referenceFrame.division.extents.ur);
        vec3 ed = eu - el;
        for (uint32 i = 0; i < 8; i++)
        {
            vec3 f = lowerUpperCombine(i).cwiseProduct(fd) + fl;
            trav->meta->cornersPhys[i] = f.cwiseProduct(ed) + el;
        }
    }

    // aabb
    if (trav->nodeInfo.distanceFromRoot() > 2)
    {
        trav->meta->aabbPhys[0]
                = trav->meta->aabbPhys[1]
                = trav->meta->cornersPhys[0];
        for (const vec3 &it : trav->meta->cornersPhys)
        {
            trav->meta->aabbPhys[0] = min(trav->meta->aabbPhys[0], it);
            trav->meta->aabbPhys[1] = max(trav->meta->aabbPhys[1], it);
        }
    }

    // surrogate
    if (vtslibs::vts::GeomExtents::validSurrogate(
                node->geomExtents.surrogate))
    {
        vec2 exU = vecFromUblas<vec2>(trav->nodeInfo.extents().ur);
        vec2 exL = vecFromUblas<vec2>(trav->nodeInfo.extents().ll);
        vec3 sds = vec2to3((exU + exL) * 0.5,
                           node->geomExtents.surrogate);
        trav->meta->surrogatePhys = convertor->convert(sds,
                            trav->nodeInfo.node(), Srs::Physical);
        trav->meta->surrogateNav = convertor->convert(sds,
                            trav->nodeInfo.node(), Srs::Navigation)[2];
    }

    // surface
    if (topmost)
    {
        trav->surface = topmost;
        // credits
        for (auto it : node->credits())
            trav->meta->credits.push_back(it);
    }

    // prepare children
    vtslibs::vts::Children childs = vtslibs::vts::children(nodeId);
    for (uint32 i = 0; i < 4; i++)
    {
        if (childsAvailable[i])
            trav->childs.push_back(std::make_shared<TraverseNode>(
                    trav->layer, trav, trav->nodeInfo.child(childs[i])));
    }

    // update priority
    trav->priority = computeResourcePriority(trav);

    // prefetch internal textures
    if (node->geometry())
    {
        for (uint32 i = 0; i < node->internalTextureCount(); i++)
            travInternalTexture(trav, i);
    }

    return true;
}

bool MapImpl::travDetermineDraws(TraverseNode *trav)
{
    assert(trav->meta);
    assert(trav->surface);
    assert(trav->rendersEmpty());

    // statistics
    statistics.currentNodeDrawsUpdates++;

    // update priority
    trav->priority = computeResourcePriority(trav);

    const TileId nodeId = trav->nodeInfo.nodeId();

    // aggregate mesh
    std::string meshAggName = trav->surface->surface->urlMesh(
            UrlTemplate::Vars(nodeId, vtslibs::vts::local(trav->nodeInfo)));
    std::shared_ptr<MeshAggregate> meshAgg = getMeshAggregate(meshAggName);
    meshAgg->updatePriority(trav->priority);
    switch (getResourceValidity(meshAggName))
    {
    case Validity::Invalid:
        trav->surface = nullptr;
        // no break here
    case Validity::Indeterminate:
        return false;
    case Validity::Valid:
        break;
    }

    bool determined = true;
    std::vector<RenderTask> newOpaque;
    std::vector<RenderTask> newTransparent;
    std::vector<vtslibs::registry::CreditId> newCredits;

    // iterate over all submeshes
    for (uint32 subMeshIndex = 0, e = meshAgg->submeshes.size();
         subMeshIndex != e; subMeshIndex++)
    {
        const MeshPart &part = meshAgg->submeshes[subMeshIndex];
        std::shared_ptr<GpuMesh> mesh = part.renderable;

        // external bound textures
        if (part.externalUv)
        {
            BoundParamInfo::List bls = trav->layer->boundList(
                        trav->surface, part.surfaceReference);
            if (part.textureLayer)
            {
                bls.push_back(BoundParamInfo(
                        vtslibs::registry::View::BoundLayerParams(
                        mapConfig->boundLayers.get(part.textureLayer).id)));
            }
            switch (reorderBoundLayers(trav->nodeInfo, subMeshIndex,
                                       bls, trav->priority))
            {
            case Validity::Indeterminate:
                determined = false;
                // no break here
            case Validity::Invalid:
                continue;
            case Validity::Valid:
                break;
            }
            bool allTransparent = true;
            for (BoundParamInfo &b : bls)
            {
                // credits
                {
                    BoundInfo *l = b.bound;
                    assert(l);
                    for (auto &it : l->credits)
                    {
                        auto c = renderer.credits.find(it.first);
                        if (c)
                            newCredits.push_back(*c);
                    }
                }

                // draw task
                RenderTask task;
                task.textureColor = b.textureColor;
                task.textureMask = b.textureMask;
                task.color(3) = b.alpha ? *b.alpha : 1;
                task.meshAgg = meshAgg;
                task.mesh = mesh;
                task.model = part.normToPhys;
                task.uvm = b.uvMatrix();
                task.externalUv = true;
                if (b.transparent || task.textureMask)
                    newTransparent.push_back(task);
                else
                    newOpaque.push_back(task);
                allTransparent = allTransparent && b.transparent;
            }
            if (!allTransparent)
                continue;
        }

        // internal texture
        if (part.internalUv)
        {
            RenderTask task;
            task.textureColor = travInternalTexture(trav, subMeshIndex);
            switch (getResourceValidity(task.textureColor))
            {
            case Validity::Indeterminate:
                determined = false;
                // no break here
            case Validity::Invalid:
                continue;
            case Validity::Valid:
                break;
            }
            task.meshAgg = meshAgg;
            task.mesh = mesh;
            task.model = part.normToPhys;
            task.uvm = identityMatrix3().cast<float>();
            task.externalUv = false;
            newOpaque.insert(newOpaque.begin(), task);
        }
    }

    if (determined)
    {
        assert(trav->rendersEmpty());
        std::swap(trav->opaque, newOpaque);
        std::swap(trav->transparent, newTransparent);
        trav->meta->credits.insert(trav->meta->credits.end(),
                             newCredits.begin(), newCredits.end());
        if (trav->rendersEmpty())
            trav->surface = nullptr;
    }

    return determined;
}

bool MapImpl::travInit(TraverseNode *trav, bool skipStatistics)
{
    // statistics
    if (!skipStatistics)
    {
        statistics.metaNodesTraversedTotal++;
        statistics.metaNodesTraversedPerLod[
                std::min<uint32>(trav->nodeInfo.nodeId().lod,
                                 MapStatistics::MaxLods-1)]++;
    }

    // update trav
    trav->lastAccessTime = renderer.tickIndex;

    // priority
    trav->priority = trav->meta
            ? computeResourcePriority(trav)
            : trav->parent
              ? trav->parent->priority
              : 0;

    // prepare meta data
    if (!trav->meta)
        return travDetermineMeta(trav);

    return true;
}

void MapImpl::travModeHierarchical(TraverseNode *trav, bool loadOnly)
{
    if (!travInit(trav))
        return;

    touchDraws(trav);
    if (trav->surface && trav->rendersEmpty())
        travDetermineDraws(trav);

    if (loadOnly)
        return;

    if (!visibilityTest(trav))
        return;

    if (coarsenessTest(trav) || trav->childs.empty())
    {
        renderNode(trav);
        return;
    }

    bool ok = true;
    for (auto &t : trav->childs)
    {
        if (!t->meta)
        {
            ok = false;
            continue;
        }
        if (t->surface && t->rendersEmpty())
            ok = false;
    }

    for (auto &t : trav->childs)
        travModeHierarchical(t.get(), !ok);

    if (!ok && !trav->rendersEmpty())
        renderNode(trav);
}

void MapImpl::travModeFlat(TraverseNode *trav)
{
    if (!travInit(trav))
        return;

    if (!visibilityTest(trav))
    {
        trav->clearRenders();
        return;
    }

    if (coarsenessTest(trav) || trav->childs.empty())
    {
        touchDraws(trav);
        if (trav->surface && trav->rendersEmpty())
            travDetermineDraws(trav);
        if (!trav->rendersEmpty())
            renderNode(trav);
        return;
    }

    trav->clearRenders();

    for (auto &t : trav->childs)
        travModeFlat(t.get());
}

void MapImpl::travModeBalanced(TraverseNode *trav)
{
    if (!travInit(trav))
        return;

    if (!visibilityTest(trav))
    {
        trav->clearRenders();
        return;
    }

    double coar = coarsenessValue(trav);

    if (coar < options.maxTexelToPixelScale
            + options.maxTexelToPixelScaleBalancedAddition)
    {
        touchDraws(trav);
        if (trav->surface && trav->rendersEmpty())
            travDetermineDraws(trav);
    }
    else if (trav->lastRenderTime + 5 < renderer.tickIndex)
        trav->clearRenders();

    bool childsHaveMeta = true;
    for (auto &it : trav->childs)
        childsHaveMeta = childsHaveMeta && travInit(it.get(), true);

    if (coar < options.maxTexelToPixelScale || trav->childs.empty()
            || !childsHaveMeta)
    {
        if (!trav->rendersEmpty() && trav->rendersReady())
            renderNode(trav);
        else
            renderNodePartialRecursive(trav);
        return;
    }

    for (auto &t : trav->childs)
        travModeBalanced(t.get());
}

void MapImpl::traverseRender(TraverseNode *trav)
{
    switch (options.traverseMode)
    {
    case TraverseMode::Hierarchical:
        travModeHierarchical(trav, false);
        break;
    case TraverseMode::Flat:
        travModeFlat(trav);
        break;
    case TraverseMode::Balanced:
        travModeBalanced(trav);
        break;
    }
}

void MapImpl::traverseClearing(TraverseNode *trav)
{
    if (trav->lastAccessTime + 5 < renderer.tickIndex)
    {
        trav->clearAll();
        return;
    }

    for (auto &it : trav->childs)
        traverseClearing(it.get());
}

} // namespace vts
