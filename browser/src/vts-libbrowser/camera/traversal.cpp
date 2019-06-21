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
#include "../gpuResource.hpp"
#include "../renderTasks.hpp"
#include "../geodata.hpp"
#include "../coordsManip.hpp"
#include "../metaTile.hpp"
#include "../mapLayer.hpp"
#include "../mapConfig.hpp"
#include "../map.hpp"

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

double CameraImpl::travDistance(TraverseNode *trav, const vec3 pointPhys)
{
    // checking the distance in node srs may be more accurate,
    //   but the resulting distance is in different units
    /*
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
    */
    return aabbPointDist(pointPhys, trav->aabbPhys[0],
            trav->aabbPhys[1]);
}

void CameraImpl::updateNodePriority(TraverseNode *trav)
{
    if (trav->meta)
    {
        // only update every 4th render frame
        if ((trav->hash + map->renderTickIndex) % 4 == 0)
        {
            trav->priority = (float)(1e6
                / (travDistance(trav, focusPosPhys) + 1));
        }
    }
    else if (trav->parent)
        trav->priority = trav->parent->priority;
    else
        trav->priority = 0;
}

std::shared_ptr<GpuTexture> CameraImpl::travInternalTexture(TraverseNode *trav,
                                                       uint32 subMeshIndex)
{
    UrlTemplate::Vars vars(trav->id(),
            vtslibs::vts::local(trav->nodeInfo), subMeshIndex);
    std::shared_ptr<GpuTexture> res = map->getTexture(
                trav->surface->urlIntTex(vars));
    map->touchResource(res);
    res->updatePriority(trav->priority);
    return res;
}

bool CameraImpl::generateMonolithicGeodataTrav(TraverseNode *trav)
{
    assert(!!trav->layer->freeLayer);
    assert(!!trav->layer->freeLayerParams);

    const vtslibs::registry::FreeLayer::Geodata &g
        = boost::get<vtslibs::registry::FreeLayer::Geodata>(
            trav->layer->freeLayer->definition);

    trav->meta.emplace();

    // extents
    {
        if (g.extents.ll != g.extents.ur)
        {
            vec3 el = vecFromUblas<vec3>
                (map->mapconfig->referenceFrame.division.extents.ll);
            vec3 eu = vecFromUblas<vec3>
                (map->mapconfig->referenceFrame.division.extents.ur);
            vec3 ed = eu - el;
            ed = vec3(1 / ed[0], 1 / ed[1], 1 / ed[2]);
            trav->meta->extents.ll = vecToUblas<math::Point3>(
                (vecFromUblas<vec3>(g.extents.ll) - el).cwiseProduct(ed));
            trav->meta->extents.ur = vecToUblas<math::Point3>(
                (vecFromUblas<vec3>(g.extents.ur) - el).cwiseProduct(ed));
            trav->aabbPhys[0] = vecFromUblas<vec3>(g.extents.ll);
            trav->aabbPhys[1] = vecFromUblas<vec3>(g.extents.ur);
        }
        else
        {
            const auto &e = map->mapconfig->referenceFrame.division.extents;
            trav->meta->extents = e;
            trav->aabbPhys[0] = vecFromUblas<vec3>(e.ll);
            trav->aabbPhys[1] = vecFromUblas<vec3>(e.ur);
        }
    }

    // other
    trav->meta->displaySize = g.displaySize;
    trav->meta->update(vtslibs::vts::MetaNode::Flag::applyDisplaySize);
    travDetermineMetaImpl(trav); // update physical corners
    trav->surface = &trav->layer->surfaceStack.surfaces[0];
    updateNodePriority(trav);
    return true;
}

bool CameraImpl::travDetermineMeta(TraverseNode *trav)
{
    assert(trav->layer);
    assert(!trav->meta);
    assert(trav->childs.empty());
    assert(!trav->determined);
    assert(trav->rendersEmpty());
    assert(!trav->parent || trav->parent->meta);

    // statistics
    statistics.currentNodeMetaUpdates++;

    // handle non-tiled geodata
    if (trav->layer->freeLayer
            && trav->layer->freeLayer->type
                == vtslibs::registry::FreeLayer::Type::geodata)
        return generateMonolithicGeodataTrav(trav);

    const TileId nodeId = trav->id();

    // find all metatiles
    std::vector<std::shared_ptr<MetaTile>> metaTiles;
    metaTiles.resize(trav->layer->surfaceStack.surfaces.size());
    const UrlTemplate::Vars tileIdVars(map->roundId(nodeId));
    bool determined = true;
    for (uint32 i = 0, e = metaTiles.size(); i != e; i++)
    {
        if (trav->parent)
        {
            const std::shared_ptr<MetaTile> &p
                    = trav->parent->metaTiles[i];
            if (!p)
                continue;
            TileId pid = vtslibs::vts::parent(nodeId);
            uint32 idx = (nodeId.x % 2) + (nodeId.y % 2) * 2;
            const vtslibs::vts::MetaNode &node = p->get(pid);
            if ((node.flags()
                 & (vtslibs::vts::MetaNode::Flag::ulChild << idx)) == 0)
                continue;
        }
        auto m = map->getMetaTile(trav->layer->surfaceStack.surfaces[i]
                             .urlMeta(tileIdVars));
        // metatiles have higher priority than other resources
        m->updatePriority(trav->priority * 2);
        switch (map->getResourceValidity(m))
        {
        case Validity::Indeterminate:
            determined = false;
            UTILITY_FALLTHROUGH;
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
    SurfaceInfo *topmost = nullptr;
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
                != trav->layer->surfaceStack.surfaces[i].alien)
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
                topmost = &trav->layer->surfaceStack.surfaces[i];
        }
        if (!node)
            node = &n;
    }
    if (!node)
        return false; // all surfaces failed to download, what can i do?

    trav->meta = *node;
    trav->metaTiles.swap(metaTiles);
    travDetermineMetaImpl(trav);

    // surface
    if (topmost)
    {
        trav->surface = topmost;
        // credits
        for (auto it : node->credits())
            trav->credits.push_back(it);
    }

    // prepare children
    vtslibs::vts::Children childs = vtslibs::vts::children(nodeId);
    for (uint32 i = 0; i < 4; i++)
    {
        if (childsAvailable[i])
            trav->childs.push_back(std::make_unique<TraverseNode>(
                    trav->layer, trav, trav->nodeInfo.child(childs[i])));
    }

    // update priority
    updateNodePriority(trav);

    return true;
}

void CameraImpl::travDetermineMetaImpl(TraverseNode *trav)
{
    assert(trav->meta);

    // corners
    if (!vtslibs::vts::empty(trav->meta->geomExtents)
            && !trav->nodeInfo.srs().empty())
    {
        vec2 fl = vecFromUblas<vec2>(trav->nodeInfo.extents().ll);
        vec2 fu = vecFromUblas<vec2>(trav->nodeInfo.extents().ur);
        vec3 el = vec2to3(fl, double(trav->meta->geomExtents.z.min));
        vec3 eu = vec2to3(fu, double(trav->meta->geomExtents.z.max));
        vec3 ed = eu - el;
        vec3 *corners = trav->cornersPhys;
        for (uint32 i = 0; i < 8; i++)
        {
            vec3 f = lowerUpperCombine(i).cwiseProduct(ed) + el;
            f = map->convertor->convert(f,
                    trav->nodeInfo.node(), Srs::Physical);
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
                vec3 p = vec4to3(vec4(t * vec3to4(corners[i], 1)), false);
                obb.points[0] = min(obb.points[0], p);
                obb.points[1] = max(obb.points[1], p);
            }

            trav->obb = obb;
        }

        // disks
        if (trav->nodeInfo.distanceFromRoot() > 4)
        {
            vec2 sds2 = vec2((fu + fl) * 0.5);
            vec3 sds = vec2to3(sds2, double(trav->meta->geomExtents.z.min));
            vec3 vn1 = map->convertor->convert(sds,
                trav->nodeInfo.node(), Srs::Physical);
            trav->diskNormalPhys = vn1.normalized();
            trav->diskHeightsPhys[0] = vn1.norm();
            sds = vec2to3(sds2, double(trav->meta->geomExtents.z.max));
            vec3 vn2 = map->convertor->convert(sds,
                trav->nodeInfo.node(), Srs::Physical);
            trav->diskHeightsPhys[1] = vn2.norm();
            sds = vec2to3(fu, double(trav->meta->geomExtents.z.min));
            vec3 vc = map->convertor->convert(sds,
                trav->nodeInfo.node(), Srs::Physical);
            trav->diskHalfAngle = std::acos(dot(trav->diskNormalPhys,
                                                vc.normalized()));
        }
    }
    else if (trav->meta->extents.ll != trav->meta->extents.ur)
    {
        vec3 fl = vecFromUblas<vec3>(trav->meta->extents.ll);
        vec3 fu = vecFromUblas<vec3>(trav->meta->extents.ur);
        vec3 fd = fu - fl;
        vec3 el = vecFromUblas<vec3>
                (map->mapconfig->referenceFrame.division.extents.ll);
        vec3 eu = vecFromUblas<vec3>
                (map->mapconfig->referenceFrame.division.extents.ur);
        vec3 ed = eu - el;
        for (uint32 i = 0; i < 8; i++)
        {
            vec3 f = lowerUpperCombine(i).cwiseProduct(fd) + fl;
            trav->cornersPhys[i] = f.cwiseProduct(ed) + el;
        }
    }
    else
    {
        LOG(warn3) << "Tile <" << trav->nodeInfo.nodeId()
            << "> is missing both extents and geomExtents";
    }

    // aabb
    if (trav->nodeInfo.distanceFromRoot() > 2)
    {
        trav->aabbPhys[0] = trav->aabbPhys[1] = trav->cornersPhys[0];
        for (const vec3 &it : trav->cornersPhys)
        {
            trav->aabbPhys[0] = min(trav->aabbPhys[0], it);
            trav->aabbPhys[1] = max(trav->aabbPhys[1], it);
        }
    }

    // surrogate
    if (vtslibs::vts::GeomExtents::validSurrogate(
                trav->meta->geomExtents.surrogate))
    {
        vec2 fl = vecFromUblas<vec2>(trav->nodeInfo.extents().ll);
        vec2 fu = vecFromUblas<vec2>(trav->nodeInfo.extents().ur);
        vec3 sds = vec2to3(vec2((fl + fu) * 0.5),
                           double(trav->meta->geomExtents.surrogate));
        trav->surrogatePhys = map->convertor->convert(sds,
                            trav->nodeInfo.node(), Srs::Physical);
        trav->surrogateNav = map->convertor->convert(sds,
                            trav->nodeInfo.node(), Srs::Navigation)[2];
    }

    // texelSize
    {
        bool applyTexelSize = trav->meta->flags()
            & vtslibs::vts::MetaNode::Flag::applyTexelSize;
        bool applyDisplaySize = trav->meta->flags()
            & vtslibs::vts::MetaNode::Flag::applyDisplaySize;

        if (applyTexelSize)
        {
            trav->texelSize = trav->meta->texelSize;
        }
        else if (applyDisplaySize)
        {
            vec3 s = trav->aabbPhys[1] - trav->aabbPhys[0];
            double m = std::max(s[0], std::max(s[1], s[2]));
            trav->texelSize = m / trav->meta->displaySize;
        }
        else
        {
            // the test fails by default
            trav->texelSize = std::numeric_limits<double>::infinity();
        }
    }
}

bool CameraImpl::travDetermineDraws(TraverseNode *trav)
{
    assert(trav->meta);
    touchDraws(trav);
    if (!trav->surface || trav->determined)
        return trav->determined;
    assert(trav->rendersEmpty());

    // statistics
    statistics.currentNodeDrawsUpdates++;

    // update priority
    updateNodePriority(trav);

    if (trav->layer->isGeodata())
        return trav->determined = travDetermineDrawsGeodata(trav);
    else
        return trav->determined = travDetermineDrawsSurface(trav);
}

bool CameraImpl::travDetermineDrawsSurface(TraverseNode *trav)
{
    const TileId nodeId = trav->id();

    // aggregate mesh
    if (!trav->meshAgg)
    {
        std::string name = trav->surface->urlMesh(
            UrlTemplate::Vars(nodeId, vtslibs::vts::local(trav->nodeInfo)));
        trav->meshAgg = map->getMeshAggregate(name);

        // prefetch internal textures
        if (trav->meta->geometry())
        {
            auto cnt = trav->meta->internalTextureCount();
            for (uint32 i = 0; i < cnt; i++)
                travInternalTexture(trav, i);
        }
    }
    auto &meshAgg = trav->meshAgg;
    meshAgg->updatePriority(trav->priority);
    switch (map->getResourceValidity(meshAgg))
    {
    case Validity::Invalid:
        trav->surface = nullptr;
        trav->touchResource = nullptr;
        trav->meshAgg = nullptr;
        return false;
    case Validity::Indeterminate:
        return false;
    case Validity::Valid:
        trav->touchResource = meshAgg;
        break;
    }

    bool determined = true;
    std::vector<RenderSurfaceTask> newOpaque;
    std::vector<RenderSurfaceTask> newTransparent;
    std::vector<vtslibs::registry::CreditId> newCredits;

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
                    map->mapconfig->boundLayers.get(part.textureLayer).id)));
            }
            switch (reorderBoundLayers(trav->nodeInfo, subMeshIndex,
                                       bls, trav->priority))
            {
            case Validity::Indeterminate:
                determined = false;
                continue;
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
                    const BoundInfo *l = b.bound;
                    assert(l);
                    for (auto &it : l->credits)
                    {
                        auto c = map->credits->find(it.first);
                        if (c)
                            newCredits.push_back(*c);
                    }
                }

                // draw task
                RenderSurfaceTask task;
                task.textureColor = b.textureColor;
                task.textureMask = b.textureMask;
                task.color(3) = b.alpha ? *b.alpha : 1;
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
            RenderSurfaceTask task;
            task.textureColor = travInternalTexture(trav, subMeshIndex);
            switch (map->getResourceValidity(task.textureColor))
            {
            case Validity::Indeterminate:
                determined = false;
                continue;
            case Validity::Invalid:
                continue;
            case Validity::Valid:
                break;
            }
            task.mesh = mesh;
            task.model = part.normToPhys;
            task.uvm = identityMatrix3().cast<float>();
            task.externalUv = false;
            newOpaque.insert(newOpaque.begin(), task);
        }
    }

    assert(!trav->determined);
    assert(trav->rendersEmpty());
    assert(trav->colliders.empty());

    if (determined)
    {
        // renders
        std::swap(trav->opaque, newOpaque);
        std::swap(trav->transparent, newTransparent);

        // colliders
        for (uint32 subMeshIndex = 0, e = meshAgg->submeshes.size();
            subMeshIndex != e; subMeshIndex++)
        {
            const MeshPart &part = meshAgg->submeshes[subMeshIndex];
            std::shared_ptr<GpuMesh> mesh = part.renderable;
            RenderSimpleTask task;
            task.mesh = mesh;
            task.model = part.normToPhys;
            trav->colliders.push_back(task);
        }

        // credits
        trav->credits.insert(trav->credits.end(),
                             newCredits.begin(), newCredits.end());

        // discard temporary
        trav->meshAgg = nullptr;
    }

    return determined;
}

bool CameraImpl::travDetermineDrawsGeodata(TraverseNode *trav)
{
    const TileId nodeId = trav->id();
    std::string geoName = trav->surface->urlGeodata(
            UrlTemplate::Vars(nodeId, vtslibs::vts::local(trav->nodeInfo)));

    auto style = map->getActualGeoStyle(trav->layer->freeLayerName);
    auto features = map->getActualGeoFeatures(
                trav->layer->freeLayerName, geoName, trav->priority);
    if (style.first == Validity::Invalid
            || features.first == Validity::Invalid)
    {
        trav->surface = nullptr;
        return false;
    }
    if (style.first == Validity::Indeterminate
            || features.first == Validity::Indeterminate)
        return false;

    std::shared_ptr<GeodataTile> geo = map->getGeodata(geoName + "#$!tile");
    geo->updatePriority(trav->priority);
    geo->update(style.second, features.second,
        map->mapconfig->browserOptions.value,
        trav->aabbPhys, trav->id().lod);
    switch (map->getResourceValidity(geo))
    {
    case Validity::Invalid:
        trav->surface = nullptr;
        trav->touchResource = nullptr;
        return false;
    case Validity::Indeterminate:
        return false;
    case Validity::Valid:
        trav->touchResource = geo;
        break;
    }

    // determined
    assert(!trav->determined);
    assert(trav->rendersEmpty());
    for (auto it : geo->renders)
        trav->geodata.push_back(it);

    return true;
}

bool CameraImpl::travInit(TraverseNode *trav)
{
    // statistics
    {
        statistics.metaNodesTraversedTotal++;
        statistics.metaNodesTraversedPerLod[
                std::min<uint32>(trav->id().lod,
                                 CameraStatistics::MaxLods-1)]++;
    }

    // update trav
    trav->lastAccessTime = map->renderTickIndex;
    updateNodePriority(trav);

    // prepare meta data
    if (!trav->meta)
        return travDetermineMeta(trav);

    return true;
}

void CameraImpl::travModeHierarchical(TraverseNode *trav, bool loadOnly)
{
    if (!travInit(trav))
        return;

    // the resources may not be unloaded
    trav->lastRenderTime = trav->lastAccessTime;

    travDetermineDraws(trav);

    if (loadOnly)
        return;

    if (!visibilityTest(trav))
        return;

    if (coarsenessTest(trav) || trav->childs.empty())
    {
        if (trav->determined)
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
        if (t->surface && !t->determined)
            ok = false;
    }

    for (auto &t : trav->childs)
        travModeHierarchical(t.get(), !ok);

    if (!ok && trav->determined)
        renderNode(trav);
}

void CameraImpl::travModeFlat(TraverseNode *trav)
{
    if (!travInit(trav))
        return;

    if (!visibilityTest(trav))
        return;

    if (coarsenessTest(trav) || trav->childs.empty())
    {
        if (travDetermineDraws(trav))
            renderNode(trav);
        return;
    }

    for (auto &t : trav->childs)
        travModeFlat(t.get());
}

// mode == 0 -> default
// mode == 1 -> load only -> returns true if loaded
// mode == 2 -> render only
bool CameraImpl::travModeStable(TraverseNode *trav, int mode)
{
    if (mode == 2)
    {
        if (!trav->meta)
            return false;
        trav->lastAccessTime = map->renderTickIndex;
    }
    else
    {
        if (!travInit(trav))
            return false;
    }

    if (!visibilityTest(trav))
        return true;

    if (mode == 2)
    {
        if (trav->determined)
        {
            touchDraws(trav);
            renderNode(trav);
        }
        else for (auto &t : trav->childs)
            travModeStable(t.get(), 2);
        return true;
    }

    if (coarsenessTest(trav) || trav->childs.empty())
    {
        travDetermineDraws(trav);
        if (mode == 1)
        {
            trav->lastRenderTime = map->renderTickIndex;
            return trav->determined;
        }
        if (trav->determined)
            renderNode(trav);
        else for (auto &t : trav->childs)
            travModeStable(t.get(), 2);
        return true;
    }

    if (mode == 0 && trav->determined)
    {
        bool ok = true;
        for (auto &t : trav->childs)
            ok = travModeStable(t.get(), 1) && ok;
        if (!ok)
        {
            touchDraws(trav);
            renderNode(trav);
            return true;
        }
    }

    {
        bool ok = true;
        for (auto &t : trav->childs)
            ok = travModeStable(t.get(), mode) && ok;
        return ok;
    }
}

bool CameraImpl::travModeBalanced(TraverseNode *trav, bool renderOnly)
{
    if (renderOnly)
    {
        if (!trav->meta)
            return false;
        trav->lastAccessTime = map->renderTickIndex;
    }
    else
    {
        if (!travInit(trav))
            return false;
    }

    if (!visibilityTest(trav))
        return true;

    if (renderOnly)
    {
        if (trav->determined)
        {
            touchDraws(trav);
            renderNode(trav);
            return true;
        }
    }
    else if (coarsenessTest(trav) || trav->childs.empty())
    {
        gridPreloadRequest(trav);
        if (travDetermineDraws(trav))
        {
            renderNode(trav);
            return true;
        }
        renderOnly = true;
    }

    Array<bool, 4> oks;
    oks.resize(trav->childs.size());
    uint32 i = 0, okc = 0;
    for (auto &it : trav->childs)
    {
        bool ok = travModeBalanced(it.get(), renderOnly);
        oks[i++] = ok;
        if (ok)
            okc++;
    }
    if (okc == 0 && renderOnly)
        return false;
    i = 0;
    for (auto &it : trav->childs)
    {
        if (!oks[i++])
            renderNodeCoarser(it.get());
    }
    return true;
}

void CameraImpl::travModeFixed(TraverseNode *trav)
{
    if (!travInit(trav))
        return;

    if (travDistance(trav, focusPosPhys) > options.fixedTraversalDistance)
        return;

    if (trav->id().lod >= options.fixedTraversalLod
        || trav->childs.empty())
    {
        if (travDetermineDraws(trav))
            renderNode(trav);
        return;
    }

    for (auto &t : trav->childs)
        travModeFixed(t.get());
}

void CameraImpl::traverseRender(TraverseNode *trav)
{
    switch (trav->layer->isGeodata() ? options.traverseModeGeodata
                                     : options.traverseModeSurfaces)
    {
    case TraverseMode::None:
        break;
    case TraverseMode::Flat:
        travModeFlat(trav);
        break;
    case TraverseMode::Stable:
        travModeStable(trav, 0);
        break;
    case TraverseMode::Balanced:
        travModeBalanced(trav, false);
        break;
    case TraverseMode::Hierarchical:
        travModeHierarchical(trav, false);
        break;
    case TraverseMode::Fixed:
        travModeFixed(trav);
        break;
    default:
        assert(false);
    }
}

} // namespace vts
