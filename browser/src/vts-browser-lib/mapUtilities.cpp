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

using vtslibs::registry::View;
using vtslibs::registry::BoundLayer;

BoundParamInfo::BoundParamInfo(const View::BoundLayerParams &params)
    : View::BoundLayerParams(params), orig(0), vars(0),
      bound(nullptr), depth(0), watertight(true), transparent(false)
{}

const mat3f BoundParamInfo::uvMatrix() const
{
    int dep = depth;
    if (dep == 0)
        return upperLeftSubMatrix(identityMatrix()).cast<float>();
    double scale = 1.0 / (1 << dep);
    double tx = scale * (orig.localId.x
                         - ((orig.localId.x >> dep) << dep));
    double ty = scale * (orig.localId.y
                         - ((orig.localId.y >> dep) << dep));
    ty = 1 - scale - ty;
    mat3f m;
    m << scale, 0, tx,
            0, scale, ty,
            0, 0, 1;
    return m;
}

Validity BoundParamInfo::prepare(const NodeInfo &nodeInfo, MapImpl *impl,
                             uint32 subMeshIndex, double priority)
{    
    bound = impl->mapConfig->getBoundInfo(id);
    if (!bound)
        return Validity::Indeterminate;

    // check lodRange and tileRange
    {
        TileId t = nodeInfo.nodeId();
        int m = bound->lodRange.min;
        if (t.lod < m)
            return Validity::Invalid;
        t.x >>= t.lod - m;
        t.y >>= t.lod - m;
        if (t.x < bound->tileRange.ll[0] || t.x > bound->tileRange.ur[0])
            return Validity::Invalid;
        if (t.y < bound->tileRange.ll[1] || t.y > bound->tileRange.ur[1])
            return Validity::Invalid;
    }

    orig = vars = UrlTemplate::Vars(nodeInfo.nodeId(), local(nodeInfo),
                                    subMeshIndex);

    depth = std::max(nodeInfo.nodeId().lod - bound->lodRange.max, 0);
    if (depth > 0)
    {
        vars.tileId.lod -= depth;
        vars.tileId.x >>= depth;
        vars.tileId.y >>= depth;
        vars.localId.lod -= depth;
        vars.localId.x >>= depth;
        vars.localId.y >>= depth;
    }

    // bound meta node
    if (bound->metaUrl)
    {
        UrlTemplate::Vars v(vars);
        v.tileId.x &= ~255;
        v.tileId.y &= ~255;
        v.localId.x &= ~255;
        v.localId.y &= ~255;
        std::string boundName = bound->urlMeta(v);
        std::shared_ptr<BoundMetaTile> bmt = impl->getBoundMetaTile(boundName);
        bmt->updatePriority(priority);
        switch (impl->getResourceValidity(bmt))
        {
        case Validity::Indeterminate:
            return Validity::Indeterminate;
        case Validity::Invalid:
            return Validity::Invalid;
        case Validity::Valid:
            break;
        }
        uint8 f = bmt->flags[(vars.tileId.y & 255) * 256
                + (vars.tileId.x & 255)];
        if ((f & BoundLayer::MetaFlags::available)
                != BoundLayer::MetaFlags::available)
            return Validity::Invalid;
        watertight = (f & BoundLayer::MetaFlags::watertight)
                == BoundLayer::MetaFlags::watertight;
    }

    transparent = bound->isTransparent || (!!alpha && *alpha < 1);

    return Validity::Valid;
}

DrawTask::DrawTask() :
    mesh(nullptr), texColor(nullptr), texMask(nullptr),
    externalUv(false), transparent(false)
{}

DrawTask::DrawTask(RenderTask *r, MapImpl *m) :
    mesh(nullptr), texColor(nullptr), texMask(nullptr)
{
    mesh = r->mesh->info.userData;
    if (r->textureColor)
        texColor = r->textureColor->info.userData;
    if (r->textureMask)
        texMask = r->textureMask->info.userData;
    mat4f mvp = (m->renderer.viewProjRender * r->model).cast<float>();
    memcpy(this->mvp, mvp.data(), sizeof(mvp));
    memcpy(uvm, r->uvm.data(), sizeof(uvm));
    memcpy(color, r->color.data(), sizeof(color));
    externalUv = r->externalUv;
    transparent = r->transparent;
}

MapDraws::MapDraws()
{
    draws.reserve(2000);
}

RenderTask::RenderTask() : model(identityMatrix()),
    uvm(upperLeftSubMatrix(identityMatrix()).cast<float>()),
    color(1,1,1,1), externalUv(false), transparent(false)
{}

bool RenderTask::ready() const
{
    if (meshAgg && !*meshAgg)
        return false;
    if (!*mesh)
        return false;
    if (textureColor && !*textureColor)
        return false;
    if (textureMask && !*textureMask)
        return false;
    return true;
}

TraverseNode::MetaInfo::MetaInfo() :
    surrogatePhys(std::numeric_limits<double>::quiet_NaN(),
                  std::numeric_limits<double>::quiet_NaN(),
                  std::numeric_limits<double>::quiet_NaN()),
    surface(nullptr), flags(0),
    surrogateValue(vtslibs::vts::GeomExtents::invalidSurrogate),
    texelSize(0), displaySize(0)
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

TraverseNode::TraverseNode(const NodeInfo &nodeInfo)
    : nodeInfo(nodeInfo), lastAccessTime(0),
      priority(std::numeric_limits<double>::quiet_NaN())
{
    instanceCounter++;
}

TraverseNode::~TraverseNode()
{
    instanceCounter--;
}

double TraverseNode::distancePhys(const vec3 &point) const
{
    assert(meta);
    double dist = std::numeric_limits<double>::infinity();
    for (auto &it : meta->cornersPhys)
        dist = std::min(dist, length(vec3(it - point)));
    return dist;
}

std::atomic<sint32> TraverseNode::instanceCounter;

void TraverseNode::clear()
{
    meta.reset();
    childs.clear();
    draws.clear();
}

bool TraverseNode::ready() const
{
    for (auto &&it : draws)
        if (!it->ready())
            return false;
    return true;
}

float MapImpl::computeResourcePriority(
        const std::shared_ptr<TraverseNode> &trav)
{
    return (float)(1e6 / (trav->distancePhys(renderer.focusPosPhys) + 1));
}

TraverseQueueItem::TraverseQueueItem(const std::shared_ptr<TraverseNode> &trav,
                                     bool loadOnly) :
    trav(trav), loadOnly(loadOnly)
{}

bool TraverseQueueItem::operator < (const TraverseQueueItem &other) const
{
    return trav->priority < other.trav->priority;
}

void MapImpl::emptyTraverseQueue()
{
    while (!renderer.traverseQueue.empty())
        renderer.traverseQueue.pop();
}

ExternalBoundLayer::ExternalBoundLayer(MapImpl *map, const std::string &name)
    : Resource(map, name, FetchTask::ResourceType::BoundLayerConfig)
{
    priority = std::numeric_limits<float>::infinity();
}

void ExternalBoundLayer::load()
{
    detail::Wrapper w(contentData);
    *(vtslibs::registry::BoundLayer*)this
            = vtslibs::registry::loadBoundLayer(w, name);
}

TilesetMapping::TilesetMapping(MapImpl *map, const std::string &name) :
    Resource(map, name, FetchTask::ResourceType::TilesetMappingConfig)
{
    priority = std::numeric_limits<float>::infinity();
}

void TilesetMapping::load()
{
    LOG(info2) << "Loading tileset mapping <" << name << ">";
    dataRaw = vtslibs::vts::deserializeTsMap(std::string(contentData.data(),
                                                         contentData.size()));
}

void TilesetMapping::update()
{
    surfaceStack.clear();
    surfaceStack.reserve(dataRaw.size() + 1);
    // the sourceReference in metanodes is one-based
    surfaceStack.push_back(MapConfig::SurfaceStackItem());
    for (auto &&it : dataRaw)
    {
        if (it.size() == 1)
        { // surface
            std::vector<std::string> id;
            id.push_back(map->mapConfig->surfaces[it[0]].id);
            MapConfig::SurfaceStackItem i;
            i.surface = std::make_shared<MapConfig::SurfaceInfo>(
                    map->mapConfig->surfaces[it[0]],
                    map->mapConfig->name);
            i.surface->name = id;
            surfaceStack.push_back(i);
        }
        else
        { // glue
            std::vector<std::string> id;
            id.reserve(it.size());
            for (auto &&it2 : it)
                id.push_back(map->mapConfig->surfaces[it2].id);
            MapConfig::SurfaceStackItem i;
            i.surface = std::make_shared<MapConfig::SurfaceInfo>(
                    *map->mapConfig->findGlue(id),
                    map->mapConfig->name);
            i.surface->name = id;
            surfaceStack.push_back(i);
        }
    }
    MapConfig::colorizeSurfaceStack(surfaceStack);
}

} // namespace vts

