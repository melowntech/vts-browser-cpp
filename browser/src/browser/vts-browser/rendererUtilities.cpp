#include "map.hpp"

namespace vts
{

using vtslibs::registry::View;
using vtslibs::registry::BoundLayer;

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

DrawTask::DrawTask(RenderTask *r, MapImpl *m) :
    mesh(nullptr), texColor(nullptr), texMask(nullptr),
    alpha(1), externalUv(false)
{
    mesh = r->mesh.get();
    texColor = r->textureColor.get();
    texMask = r->textureMask.get();
    mat4f mvp = (m->renderer.viewProjRender * r->model).cast<float>();
    memcpy(this->mvp, mvp.data(), sizeof(mvp));
    memcpy(uvm, r->uvm.data(), sizeof(uvm));
    memcpy(color, r->color.data(), sizeof(color));
    externalUv = r->external;
}

RenderTask::RenderTask() : model(identityMatrix()),
    uvm(upperLeftSubMatrix(identityMatrix()).cast<float>()),
    color(1,0,0), external(false), type(Type::Invalid)
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

TraverseNode::TraverseNode(const NodeInfo &nodeInfo)
    : nodeInfo(nodeInfo), surface(nullptr), lastAccessTime(0),
      flags(0), texelSize(0), displaySize(0),
      surrogate(vtslibs::vts::GeomExtents::invalidSurrogate),
      validity(Validity::Indeterminate), empty(false)
{}

TraverseNode::~TraverseNode()
{}

void TraverseNode::clear()
{
    draws.clear();
    childs.clear();
    surface = nullptr;
    empty = false;
    if (validity == Validity::Valid)
        validity = Validity::Indeterminate;
}

bool TraverseNode::ready() const
{
    for (auto &&it : draws)
        if (!it->ready())
            return false;
    return true;
}

MapImpl::Renderer::Renderer() : metaTileBinaryOrder(0),
    windowWidth(0), windowHeight(0)
{}

} // namespace vts

