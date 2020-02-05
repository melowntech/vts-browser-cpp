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

#include "../renderInfos.hpp"
#include "../camera.hpp"
#include "../mapConfig.hpp"
#include "../map.hpp"
#include "../validity.hpp"
#include "../gpuResource.hpp"
#include "../metaTile.hpp"

namespace vts
{

using vtslibs::registry::View;
using vtslibs::registry::BoundLayer;

BoundInfo::BoundInfo(const vtslibs::registry::BoundLayer &bl,
                                const std::string &url)
    : BoundLayer(bl)
{
    urlExtTex.parse(convertPath(this->url, url));
    if (metaUrl)
    {
        urlMeta.parse(convertPath(*metaUrl, url));
        urlMask.parse(convertPath(*maskUrl, url));
    }
    if (bl.availability)
    {
        availability = std::make_shared<
        vtslibs::registry::BoundLayer::Availability>(
            *bl.availability);
    }
}

BoundParamInfo::BoundParamInfo(const View::BoundLayerParams &params)
    : View::BoundLayerParams(params)
{}

mat3f BoundParamInfo::uvMatrix() const
{
    if (depth == 0)
        return identityMatrix3().cast<float>();
    double scale = 1.0 / (1 << depth);
    double tx = scale * (orig.localId.x
                         - ((orig.localId.x >> depth) << depth));
    double ty = scale * (orig.localId.y
                         - ((orig.localId.y >> depth) << depth));
    ty = 1 - scale - ty;
    mat3f m;
    m << scale, 0, tx,
            0, scale, ty,
            0, 0, 1;
    return m;
}

Validity BoundParamInfo::prepare(const NodeInfo &nodeInfo, CameraImpl *impl,
                uint32 subMeshIndex, double priority)
{
    bound = impl->map->mapconfig->getBoundInfo(id);
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

    orig = UrlTemplate::Vars(nodeInfo.nodeId(),
                                local(nodeInfo), subMeshIndex);

    depth = std::max(nodeInfo.nodeId().lod - bound->lodRange.max, 0);

    while (true)
    {
        assert(nodeInfo.nodeId().lod - depth >= bound->lodRange.min
            && nodeInfo.nodeId().lod - depth <= bound->lodRange.max);
        switch (prepareDepth(impl, priority))
        {
        case Validity::Indeterminate:
            return Validity::Indeterminate;
        case Validity::Invalid:
            break;
        case Validity::Valid:
            return Validity::Valid;
        }
        if (nodeInfo.nodeId().lod - depth == bound->lodRange.min)
            return Validity::Invalid;
        depth++;
    }
}

Validity BoundParamInfo::prepareDepth(CameraImpl *impl, double priority)
{
    UrlTemplate::Vars vars = orig;

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
    bool watertight = true;
    if (bound->metaUrl)
    {
        UrlTemplate::Vars v(vars);
        v.tileId.x &= ~255;
        v.tileId.y &= ~255;
        v.localId.x &= ~255;
        v.localId.y &= ~255;
        std::string boundName = bound->urlMeta(v);
        std::shared_ptr<BoundMetaTile> bmt
                = impl->map->getBoundMetaTile(boundName);
        bmt->updatePriority(priority);
        switch (impl->map->getResourceValidity(bmt))
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

    textureColor = impl->map->getTexture(bound->urlExtTex(vars));
    textureColor->updatePriority(priority);
    textureColor->updateAvailability(bound->availability);
    switch (impl->map->getResourceValidity(textureColor))
    {
    case Validity::Indeterminate:
        return Validity::Indeterminate;
    case Validity::Invalid:
        return Validity::Invalid;
    case Validity::Valid:
        break;
    }
    if (!watertight)
    {
        textureMask = impl->map->getTexture(bound->urlMask(vars));
        textureMask->updatePriority(priority);
        switch (impl->map->getResourceValidity(textureMask))
        {
        case Validity::Indeterminate:
            return Validity::Indeterminate;
        case Validity::Invalid:
            return Validity::Invalid;
        case Validity::Valid:
            break;
        }
    }
    else
        textureMask.reset();

    return Validity::Valid;
}

Validity CameraImpl::reorderBoundLayers(const NodeInfo &nodeInfo,
        uint32 subMeshIndex, BoundParamInfo::List &boundList, double priority)
{
    std::reverse(boundList.begin(), boundList.end());
    auto it = boundList.begin();
    while (it != boundList.end())
    {
        bool transparent = true;
        switch (it->prepare(nodeInfo, this, subMeshIndex, priority))
        {
        case Validity::Invalid:
            it = boundList.erase(it);
            break;
        case Validity::Indeterminate:
            return Validity::Indeterminate;
        case Validity::Valid:
            transparent = !!it->textureMask || it->transparent;
            it++;
        }
        if (!transparent)
        {
            boundList.erase(it, boundList.end());
            break;
        }
    }
    std::reverse(boundList.begin(), boundList.end());
    return Validity::Valid;
}

} // namespace vts

