#include "map.h"

namespace melown
{

using vtslibs::registry::View;
using vtslibs::registry::BoundLayer;

BoundParamInfo::BoundParamInfo(const View::BoundLayerParams &params)
    : View::BoundLayerParams(params), bound(nullptr),
      watertight(true), availCache(false),
      vars(0), orig(0), impl(nullptr)
{}

bool BoundParamInfo::available(bool primaryLod)
{
    if (availCache)
        return true;  
    
    // check mask texture
    if (bound->metaUrl && !watertight)
    {
        if (primaryLod)
        {
            GpuTexture *t = impl->getTexture(bound->urlMask(vars));
            if (!*t)
                return false;
        }
        else
        {
            if (!impl->getResourceReady(bound->urlMask(vars)))
                return false;
        }
    }
    
    // check color texture
    if (primaryLod)
    {
        GpuTexture *t = impl->getTexture(bound->urlExtTex(vars));
        t->impl->availTest = bound->availability.get_ptr();
        return availCache = *t;
    }
    else
        return availCache = impl->getResourceReady(bound->urlExtTex(vars));
}

void BoundParamInfo::varsFallDown(int depth)
{
    if (depth == 0)
        return;
    vars.tileId.lod -= depth;
    vars.tileId.x >>= depth;
    vars.tileId.y >>= depth;
    vars.localId.lod -= depth;
    vars.localId.x >>= depth;
    vars.localId.y >>= depth;
}

const mat3f BoundParamInfo::uvMatrix() const
{
    int dep = depth();
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

bool BoundParamInfo::canGoUp() const
{
    return vars.localId.lod <= bound->lodRange.min + 1;
}

int BoundParamInfo::depth() const
{
    return std::max(nodeInfo->nodeId().lod - bound->lodRange.max, 0)
            + orig.localId.lod - vars.localId.lod;
}

void BoundParamInfo::prepare(const NodeInfo &nodeInfo, MapImpl *impl,
                             uint32 subMeshIndex)
{
    this->nodeInfo = &nodeInfo;
    this->impl = impl;
    bound = impl->getBoundInfo(id);
    if (!bound)
        return;
    orig = vars = UrlTemplate::Vars(nodeInfo.nodeId(), local(nodeInfo),
                                    subMeshIndex);
    varsFallDown(depth());
    
    if (bound->metaUrl)
    { // bound meta nodes
        UrlTemplate::Vars v(vars);
        v.tileId.x &= ~255;
        v.tileId.y &= ~255;
        v.localId.x &= ~255;
        v.localId.y &= ~255;
        BoundMetaTile *bmt = impl->getBoundMetaTile(bound->urlMeta(v));
        if (!*bmt)
        {
            bound = nullptr;
            return;
        }
        
        uint8 f = bmt->flags[(vars.tileId.y & 255) * 256
                + (vars.tileId.x & 255)];
        if ((f & BoundLayer::MetaFlags::available)
                != BoundLayer::MetaFlags::available)
        {
            bound = nullptr;
            return;
        }
        
        watertight = (f & BoundLayer::MetaFlags::watertight)
                == BoundLayer::MetaFlags::watertight;
    }
}

bool BoundParamInfo::invalid() const
{
    if (!bound)
        return true;
    TileId t = nodeInfo->nodeId();
    int m = bound->lodRange.min;
    if (t.lod < m)
        return true;
    t.x >>= t.lod - m;
    t.y >>= t.lod - m;
    if (t.x < bound->tileRange.ll[0] || t.x > bound->tileRange.ur[0])
        return true;
    if (t.y < bound->tileRange.ll[1] || t.y > bound->tileRange.ur[1])
        return true;
    return false;
}

bool BoundParamInfo::operator <(const BoundParamInfo &rhs) const
{
    return depth() > rhs.depth();
}

} // namespace melown

