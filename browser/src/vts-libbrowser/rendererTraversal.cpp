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

#include <boost/utility/in_place_factory.hpp>

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

bool aabbTest(const vec3 aabb[2], const vec4 planes[6])
{
    for (uint32 i = 0; i < 6; i++)
    {
        const vec4 &p = planes[i]; // current plane
        vec3 pv = vec3( // current p-vertex
                aabb[!!(p[0] > 0)](0),
                aabb[!!(p[1] > 0)](1),
                aabb[!!(p[2] > 0)](2));
        double d = dot(vec4to3(p), pv);
        if (d < -p[3])
            return false;
    }
    return true;
}

vec4 column(const mat4 &m, uint32 index)
{
    return vec4(m(index, 0), m(index, 1), m(index, 2), m(index, 3));
}

void frustumPlanes(const mat4 &vp, vec4 planes[6])
{
    vec4 c0 = column(vp, 0);
    vec4 c1 = column(vp, 1);
    vec4 c2 = column(vp, 2);
    vec4 c3 = column(vp, 3);
    planes[0] = c3 + c0;
    planes[1] = c3 - c0;
    planes[2] = c3 + c1;
    planes[3] = c3 - c1;
    planes[4] = c3 + c2;
    planes[5] = c3 - c2;
}

} // namespace

Validity MapImpl::reorderBoundLayers(const NodeInfo &nodeInfo,
        uint32 subMeshIndex, BoundParamInfo::List &boundList, double priority)
{
    // prepare all layers
    {
        bool determined = true;
        auto it = boundList.begin();
        while (it != boundList.end())
        {
            switch (it->prepare(nodeInfo, this, subMeshIndex, priority))
            {
            case Validity::Invalid:
                it = boundList.erase(it);
                break;
            case Validity::Indeterminate:
                determined = false;
                // no break here
            case Validity::Valid:
                it++;
            }
        }
        if (!determined)
            return Validity::Indeterminate;
    }

    // skip overlapping layers
    std::reverse(boundList.begin(), boundList.end());
    auto it = boundList.begin(), et = boundList.end();
    while (it != et && (!it->watertight || it->transparent))
        it++;
    if (it != et)
        boundList.erase(++it, et);
    std::reverse(boundList.begin(), boundList.end());
    
    return Validity::Valid;
}

bool MapImpl::visibilityTest(TraverseNode *trav)
{
    assert(trav->meta);
    // aabb test
    if (!aabbTest(trav->meta->aabbPhys, renderer.frustumPlanes))
        return false;
    // additional obb test
    if (trav->meta->obb)
    {
        TraverseNode::Obb &obb = *trav->meta->obb;
        vec4 planes[6];
        frustumPlanes(renderer.viewProj * obb.rotInv, planes);
        if (!aabbTest(obb.points, planes))
            return false;
    }
    // all tests passed
    return true;
}

bool MapImpl::coarsenessTest(TraverseNode *trav)
{
    assert(trav->meta);
    return coarsenessValue(trav) < options.maxTexelToPixelScale;
}

double MapImpl::coarsenessValue(TraverseNode *trav)
{
    bool applyTexelSize = trav->meta->flags()
            & vtslibs::vts::MetaNode::Flag::applyTexelSize;
    bool applyDisplaySize = trav->meta->flags()
            & vtslibs::vts::MetaNode::Flag::applyDisplaySize;

    if (!applyTexelSize && !applyDisplaySize)
        return std::numeric_limits<double>::infinity();

    double result = 0;

    if (applyTexelSize)
    {
        vec3 up = renderer.perpendicularUnitVector * trav->meta->texelSize;
        for (const vec3 &c : trav->meta->cornersPhys)
        {
            vec3 c1 = c - up * 0.5;
            vec3 c2 = c1 + up;
            c1 = vec4to3(renderer.viewProj * vec3to4(c1, 1), true);
            c2 = vec4to3(renderer.viewProj * vec3to4(c2, 1), true);
            double len = std::abs(c2[1] - c1[1]) * renderer.windowHeight * 0.5;
            result = std::max(result, len);
        }
    }

    if (applyDisplaySize)
    {
        // todo
    }

    return result;
}

void MapImpl::renderNode(TraverseNode *trav, const vec4f &uvClip)
{
    assert(trav->meta);
    assert(!trav->rendersEmpty());
    assert(trav->rendersReady());
    assert(visibilityTest(trav));

    // statistics
    statistics.meshesRenderedTotal++;
    statistics.meshesRenderedPerLod[std::min<uint32>(
        trav->nodeInfo.nodeId().lod, MapStatistics::MaxLods-1)]++;

    // meshes
    if (!options.debugRenderNoMeshes)
    {
        for (const RenderTask &r : trav->opaque)
            draws.opaque.emplace_back(r, uvClip.data(), this);
        for (const RenderTask &r : trav->transparent)
            draws.transparent.emplace_back(r, uvClip.data(), this);
    }

    // surrogate
    if (options.debugRenderSurrogates)
    {
        RenderTask task;
        task.mesh = getMeshRenderable("internal://data/meshes/sphere.obj");
        task.mesh->priority = std::numeric_limits<float>::infinity();
        task.model = translationMatrix(trav->meta->surrogatePhys)
                * scaleMatrix(trav->nodeInfo.extents().size() * 0.03);
        if (trav->meta->surface)
            task.color = vec3to4f(trav->meta->surface->color, task.color(3));
        if (task.ready())
            draws.Infographic.emplace_back(task, this);
    }

    // mesh box
    if (options.debugRenderMeshBoxes)
    {
        for (RenderTask &r : trav->opaque)
        {
            RenderTask task;
            task.model = r.model;
            task.mesh = getMeshRenderable("internal://data/meshes/aabb.obj");
            task.mesh->priority = std::numeric_limits<float>::infinity();
            task.color = vec4f(0, 0, 1, 1);
            if (task.ready())
                draws.Infographic.emplace_back(task, this);
        }
    }

    // tile box
    if (options.debugRenderTileBoxes)
    {
        RenderTask task;
        task.mesh = getMeshRenderable("internal://data/meshes/line.obj");
        task.mesh->priority = std::numeric_limits<float>::infinity();
        task.color = vec4f(1, 0, 0, 1);
        if (task.ready())
        {
            static const uint32 cora[] = {
                0, 0, 1, 2, 4, 4, 5, 6, 0, 1, 2, 3
            };
            static const uint32 corb[] = {
                1, 2, 3, 3, 5, 6, 7, 7, 4, 5, 6, 7
            };
            for (uint32 i = 0; i < 12; i++)
            {
                vec3 a = trav->meta->cornersPhys[cora[i]];
                vec3 b = trav->meta->cornersPhys[corb[i]];
                task.model = lookAt(a, b);
                draws.Infographic.emplace_back(task, this);
            }
        }
    }

    // credits
    for (auto &it : trav->meta->credits)
        renderer.credits.hit(Credits::Scope::Imagery, it,
                             trav->nodeInfo.distanceFromRoot());

    trav->lastRenderTime = renderer.tickIndex;
}

namespace
{

void updateRangeToHalf(float &a, float &b, int which)
{
    a *= 0.5;
    b *= 0.5;
    if (which)
    {
        a += 0.5;
        b += 0.5;
    }
}

} // namespace

void MapImpl::renderNodePartialRecursive(TraverseNode *trav, vec4f uvClip)
{
    if (!trav->parent || !trav->parent->meta->surface)
        return;

    auto id = trav->nodeInfo.nodeId();
    float *arr = uvClip.data();
    updateRangeToHalf(arr[0], arr[2], id.x % 2);
    updateRangeToHalf(arr[1], arr[3], 1 - (id.y % 2));

    if (!trav->parent->rendersEmpty() && trav->parent->rendersReady())
        renderNode(trav->parent, uvClip);
    else
        renderNodePartialRecursive(trav->parent, uvClip);
}

std::shared_ptr<Resource> MapImpl::travInternalTexture(TraverseNode *trav,
                                                       uint32 subMeshIndex)
{
    UrlTemplate::Vars vars(trav->nodeInfo.nodeId(),
            vtslibs::vts::local(trav->nodeInfo), subMeshIndex);
    std::shared_ptr<Resource> res = getTexture(
                trav->meta->surface->surface->urlIntTex(vars));
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
    metaTiles.resize(mapConfig->surfaceStack.size());
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
        auto m = getMetaTile(mapConfig->surfaceStack[i].surface
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
    MapConfig::SurfaceStackItem *topmost = nullptr;
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
        if (topmost || n.alien() != mapConfig->surfaceStack[i].alien)
            continue;
        if (n.geometry())
        {
            node = &n;
            if (renderer.tilesetMapping)
            {
                assert(n.sourceReference > 0 && n.sourceReference
                       <= renderer.tilesetMapping->surfaceStack.size());
                topmost = &renderer.tilesetMapping
                        ->surfaceStack[n.sourceReference];
            }
            else
                topmost = &mapConfig->surfaceStack[i];
        }
        if (!node)
            node = &n;
    }
    if (!node)
        return false; // all surfaces failed to download, what can i do?

    trav->meta = std::make_shared<TraverseNode::MetaInfo>(*node);
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

            trav->meta->obb = std::make_shared<TraverseNode::Obb>(obb);
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
    }

    // surface
    if (topmost)
    {
        trav->meta->surface = topmost;
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
                        trav, trav->nodeInfo.child(childs[i])));
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
    assert(trav->meta->surface);
    assert(trav->rendersEmpty());

    // statistics
    statistics.currentNodeDrawsUpdates++;

    // update priority
    trav->priority = computeResourcePriority(trav);

    const TileId nodeId = trav->nodeInfo.nodeId();

    // aggregate mesh
    std::string meshAggName = trav->meta->surface->surface->urlMesh(
            UrlTemplate::Vars(nodeId, vtslibs::vts::local(trav->nodeInfo)));
    std::shared_ptr<MeshAggregate> meshAgg = getMeshAggregate(meshAggName);
    meshAgg->updatePriority(trav->priority);
    switch (getResourceValidity(meshAggName))
    {
    case Validity::Invalid:
        trav->meta->surface = nullptr;
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
            std::string surfaceName;
            if (trav->meta->surface->surface->name.size() > 1)
                surfaceName = trav->meta->surface->surface
                        ->name[part.surfaceReference - 1];
            else
                surfaceName = trav->meta->surface->surface->name.back();
            const vtslibs::registry::View::BoundLayerParams::list &boundList
                    = mapConfig->view.surfaces[surfaceName];
            BoundParamInfo::List bls(boundList.begin(), boundList.end());
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
                    MapConfig::BoundInfo *l = b.bound;
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
                task.textureColor = getTexture(b.bound->urlExtTex(b.vars));
                task.textureColor->updatePriority(trav->priority);
                task.textureColor->availTest = b.bound->availability;
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
                if (!b.watertight)
                {
                    task.textureMask = getTexture(b.bound->urlMask(b.vars));
                    task.textureMask->updatePriority(trav->priority);
                    switch (getResourceValidity(task.textureMask))
                    {
                    case Validity::Indeterminate:
                        determined = false;
                        // no break here
                    case Validity::Invalid:
                        continue;
                    case Validity::Valid:
                        break;
                    }
                }
                task.color(3) = b.alpha ? *b.alpha : 1;
                task.meshAgg = meshAgg;
                task.mesh = mesh;
                task.model = part.normToPhys;
                task.uvm = b.uvMatrix();
                task.externalUv = true;
                if (b.transparent)
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
            trav->meta->surface = nullptr;
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
    if (trav->meta->surface && trav->rendersEmpty())
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
        if (t->meta->surface && t->rendersEmpty())
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
        if (trav->meta->surface && trav->rendersEmpty())
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
        if (trav->meta->surface && trav->rendersEmpty())
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

void MapImpl::updateCamera()
{
    vec3 center, dir, up;
    positionToCamera(center, dir, up);

    vtslibs::registry::Position &pos = mapConfig->position;

    // camera view matrix
    double dist = pos.type == vtslibs::registry::Position::Type::objective
            ? positionObjectiveDistance() : 1e-5;
    vec3 cameraPosPhys = center - dir * dist;
    if (callbacks.cameraOverrideEye)
        callbacks.cameraOverrideEye((double*)&cameraPosPhys);
    if (callbacks.cameraOverrideTarget)
        callbacks.cameraOverrideTarget((double*)&center);
    if (callbacks.cameraOverrideUp)
        callbacks.cameraOverrideUp((double*)&up);
    mat4 view = lookAt(cameraPosPhys, center, up);
    if (callbacks.cameraOverrideView)
    {
        callbacks.cameraOverrideView((double*)&view);
        // update center, dir and up
        mat4 vi = view.inverse();
        cameraPosPhys = vec4to3(vi * vec4(0, 0, -1, 1), true);
        dir = vec4to3(vi * vec4(0, 0, -1, 0), false);
        up = vec4to3(vi * vec4(0, 1, 0, 0), false);
        center = cameraPosPhys + dir * dist;
    }

    // camera projection matrix
    double near = std::max(2.0, dist * 0.1);
    double terrainAboveOrigin = 0;
    double cameraAboveOrigin = 0;
    switch (mapConfig->navigationSrsType())
    {
    case vtslibs::registry::Srs::Type::projected:
    {
        const vtslibs::registry::Srs &srs = mapConfig->srs.get(
                    mapConfig->referenceFrame.model.navigationSrs);
        if (srs.periodicity)
            terrainAboveOrigin = srs.periodicity->period / (2 * 3.14159);
        cameraAboveOrigin = terrainAboveOrigin + dist * 2;
    } break;
    case vtslibs::registry::Srs::Type::geographic:
    {
        terrainAboveOrigin = length(convertor->navToPhys(vec2to3(vec3to2(
                                    vecFromUblas<vec3>(pos.position)), 0)));
        cameraAboveOrigin = length(cameraPosPhys);
    } break;
    case vtslibs::registry::Srs::Type::cartesian:
        LOGTHROW(fatal, std::invalid_argument) << "Invalid navigation srs type";
    }
    double cameraToHorizon = cameraAboveOrigin > terrainAboveOrigin
            ? std::sqrt(cameraAboveOrigin * cameraAboveOrigin
                - terrainAboveOrigin * terrainAboveOrigin)
            : 0;
    double mountains = 5000 + terrainAboveOrigin;
    double mountainsBehindHorizon = std::sqrt(mountains * mountains
                                    - terrainAboveOrigin * terrainAboveOrigin);
    double far = cameraToHorizon + mountainsBehindHorizon;
    double fov = pos.verticalFov;
    double aspect = (double)renderer.windowWidth/(double)renderer.windowHeight;
    if (callbacks.cameraOverrideFovAspectNearFar)
        callbacks.cameraOverrideFovAspectNearFar(fov, aspect, near, far);
    assert(fov > 1e-3 && fov < 180 - 1e-3);
    assert(aspect > 0);
    assert(near > 0);
    assert(far > near);
    mat4 proj = perspectiveMatrix(fov, aspect, near, far);
    if (callbacks.cameraOverrideProj)
        callbacks.cameraOverrideProj((double*)&proj);

    // few other variables
    renderer.viewProjRender = proj * view;
    if (!options.debugDetachedCamera)
    {
        renderer.viewProj = renderer.viewProjRender;
        renderer.perpendicularUnitVector
                = normalize(cross(cross(up, dir), dir));
        renderer.forwardUnitVector = dir;
        frustumPlanes(renderer.viewProj, renderer.frustumPlanes);
        renderer.cameraPosPhys = cameraPosPhys;
        renderer.focusPosPhys = center;
    }
    else
    {
        // render original camera
        RenderTask task;
        task.mesh = getMeshRenderable("internal://data/meshes/line.obj");
        task.mesh->priority = std::numeric_limits<float>::infinity();
        task.color = vec4f(0, 1, 0, 1);
        if (task.ready())
        {
            std::vector<vec3> corners;
            corners.reserve(8);
            mat4 m = renderer.viewProj.inverse();
            for (int x = 0; x < 2; x++)
                for (int y = 0; y < 2; y++)
                    for (int z = 0; z < 2; z++)
                        corners.push_back(vec4to3(m
                            * vec4(x * 2 - 1, y * 2 - 1, z * 2 - 1, 1), true));
            static const uint32 cora[] = {
                0, 0, 1, 2, 4, 4, 5, 6, 0, 1, 2, 3
            };
            static const uint32 corb[] = {
                1, 2, 3, 3, 5, 6, 7, 7, 4, 5, 6, 7
            };
            for (uint32 i = 0; i < 12; i++)
            {
                vec3 a = corners[cora[i]];
                vec3 b = corners[corb[i]];
                task.model = lookAt(a, b);
                draws.Infographic.emplace_back(task, this);
            }
        }
    }

    // render object position
    if (options.debugRenderObjectPosition)
    {
        vec3 phys = convertor->navToPhys(vecFromUblas<vec3>(pos.position));
        RenderTask r;
        r.mesh = getMeshRenderable("internal://data/meshes/cube.obj");
        r.mesh->priority = std::numeric_limits<float>::infinity();
        r.textureColor = getTexture("internal://data/textures/helper.jpg");
        r.textureColor->priority = std::numeric_limits<float>::infinity();
        r.model = translationMatrix(phys)
                * scaleMatrix(pos.verticalExtent * 0.015);
        if (r.ready())
            draws.Infographic.emplace_back(r, this);
    }

    // render target position
    if (options.debugRenderTargetPosition)
    {
        vec3 phys = convertor->navToPhys(navigation.targetPoint);
        RenderTask r;
        r.mesh = getMeshRenderable("internal://data/meshes/cube.obj");
        r.mesh->priority = std::numeric_limits<float>::infinity();
        r.textureColor = getTexture("internal://data/textures/helper.jpg");
        r.textureColor->priority = std::numeric_limits<float>::infinity();
        r.model = translationMatrix(phys)
                * scaleMatrix(navigation.targetViewExtent * 0.015);
        if (r.ready())
            draws.Infographic.emplace_back(r, this);
    }

    // update draws camera
    {
        MapDraws::Camera &c = draws.camera;
        matToRaw(view, c.view);
        matToRaw(proj, c.proj);
        vecToRaw(center, c.target);
        vecToRaw(cameraPosPhys, c.eye);
        c.near = near;
        c.far = far;
    }
}

} // namespace vts
