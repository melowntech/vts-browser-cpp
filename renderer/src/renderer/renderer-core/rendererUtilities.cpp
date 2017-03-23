#include "map.h"

namespace melown
{

using vtslibs::registry::View;
using vtslibs::registry::BoundLayer;

const NodeInfo HeightRequest::findPosition(
        NodeInfo &info, const vec2 &pos, double viewExtent)
{
    double desire = std::log2(4 * info.extents().size()
                              / viewExtent) - 8;
    if (desire < 1)
        return info;
    
    vtslibs::vts::Children childs = vtslibs::vts::children(info.nodeId());
    for (uint32 i = 0; i < childs.size(); i++)
    {
        NodeInfo ni = info.child(childs[i]);
        if (!ni.inside(vecToUblas<math::Point2>(pos)))
            continue;
        return findPosition(ni, pos, viewExtent);
    }
    assert(false);
}

HeightRequest::HeightRequest(const vec2 &navPos, MapImpl *map) :
    frameIndex(map->statistics.frameIndex)
{
    for (auto &&it : map->mapConfig->referenceFrame.division.nodes)
    {
        if (!it.second.valid())
            continue;
        NodeInfo ni(map->mapConfig->referenceFrame, it.first,
                    false, *map->mapConfig);
        vec2 sdp = vec3to2(map->convertor->convert(
                       vec2to3(navPos, 0), map->mapConfig
                       ->referenceFrame.model.navigationSrs, it.second.srs));
        if (!ni.inside(vecToUblas<math::Point2>(sdp)))
            continue;
        ni = findPosition(ni, sdp, map->mapConfig->position.verticalExtent);
        pixPos = NavTile::sds2px(sdp, ni.extents());
        nodeId = ni.nodeId();
        return;
    }
    assert(false);
}

BoundParamInfo::BoundParamInfo(const View::BoundLayerParams &params)
    : View::BoundLayerParams(params), bound(nullptr),
      watertight(true), vars(0), orig(0), depth(0)
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
                             uint32 subMeshIndex)
{    
    bound = impl->mapConfig->getBoundInfo(id);
    if (!bound)
        return Validity::Indeterminate;
    
    { // check lodRange and tileRange
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
    
    depth = std::max(nodeInfo.nodeId().lod - bound->lodRange.max, 0)
            + orig.localId.lod - vars.localId.lod;
    
    if (depth > 0)
    {
        vars.tileId.lod -= depth;
        vars.tileId.x >>= depth;
        vars.tileId.y >>= depth;
        vars.localId.lod -= depth;
        vars.localId.x >>= depth;
        vars.localId.y >>= depth;
    }
    
    if (bound->metaUrl)
    { // bound meta node
        UrlTemplate::Vars v(vars);
        v.tileId.x &= ~255;
        v.tileId.y &= ~255;
        v.localId.x &= ~255;
        v.localId.y &= ~255;
        std::string boundName = bound->urlMeta(v);
        std::shared_ptr<BoundMetaTile> bmt = impl->getBoundMetaTile(boundName);
        switch (impl->getResourceValidity(boundName))
        {
        case Validity::Indeterminate:
            return Validity::Indeterminate;
        case Validity::Invalid:
            return Validity::Invalid;
        }        
        uint8 f = bmt->flags[(vars.tileId.y & 255) * 256
                + (vars.tileId.x & 255)];
        if ((f & BoundLayer::MetaFlags::available)
                != BoundLayer::MetaFlags::available)
            return Validity::Invalid;
        watertight = (f & BoundLayer::MetaFlags::watertight)
                == BoundLayer::MetaFlags::watertight;
    }
    
    return Validity::Valid;
}

bool BoundParamInfo::operator <(const BoundParamInfo &rhs) const
{
    return depth > rhs.depth;
}

RenderTask::RenderTask() : model(identityMatrix()),
    uvm(upperLeftSubMatrix(identityMatrix()).cast<float>()),
    color(1,0,0), external(false)
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

RenderBatch::RenderBatch()
{}

void RenderBatch::clear()
{
    opaque.clear();
    transparent.clear();
    wires.clear();
}

void RenderBatch::mergeIn(const RenderBatch &batch, const MapOptions &options)
{
    opaque.insert(opaque.end(), batch.opaque.begin(), batch.opaque.end());
    transparent.insert(transparent.end(), batch.transparent.begin(),
                       batch.transparent.end());
    if (options.renderWireBoxes)
        wires.insert(wires.end(), batch.wires.begin(), batch.wires.end());
}

bool RenderBatch::ready() const
{
    for (auto &&it : opaque)
        if (!it->ready())
            return false;
    for (auto &&it : transparent)
        if (!it->ready())
            return false;
    return true;
}

TraverseNode::TraverseNode(const NodeInfo &nodeInfo)
    : nodeInfo(nodeInfo), surface(nullptr), lastAccessTime(0),
      validity(Validity::Indeterminate), empty(false)
{}

TraverseNode::~TraverseNode()
{}

MapImpl::Renderer::Renderer() : metaTileBinaryOrder(0),
    windowWidth(0), windowHeight(0)
{}

} // namespace melown

