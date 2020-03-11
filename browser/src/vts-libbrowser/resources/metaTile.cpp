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

#include "../metaTile.hpp"
#include "../fetchTask.hpp"
#include "../mapConfig.hpp"
#include "../map.hpp"
#include "../coordsManip.hpp"

#include <vts-libs/vts/nodeinfo.hpp>
#include <dbglog/dbglog.hpp>

namespace vts
{

MetaNode::MetaNode(const NodeInfo &nodeInfo) :
    nodeInfo(nodeInfo),
    diskNormalPhys(nan3()),
    diskHeightsPhys(nan2()),
    diskHalfAngle(nan1()),
    texelSize(nan1())
{
    // initialize corners to NAN
    {
        for (uint32 i = 0; i < 8; i++)
            cornersPhys[i] = nan3();
        surrogatePhys = nan3();
    }
    // initialize aabb to universe
    {
        double di = std::numeric_limits<double>::infinity();
        vec3 vi(di, di, di);
        aabbPhys[0] = -vi;
        aabbPhys[1] = vi;
    }
}

MetaTile::MetaTile(vts::MapImpl *map, const std::string &name) :
    Resource(map, name),
    vtslibs::vts::MetaTile(vtslibs::vts::TileId(), 0)
{
    mapconfig = map->mapconfig;
}

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

MetaNode generateMetaNode(const std::shared_ptr<Mapconfig> &m,
    const vtslibs::vts::TileId &id, const vtslibs::vts::MetaNode &meta)
{
    const auto &cnv = m->map->convertor;
    vtslibs::vts::NodeInfo nodeInfo(m->referenceFrame, id, false, *m);
    MetaNode node(nodeInfo);

    // corners
    if (!vtslibs::vts::empty(meta.geomExtents)
            && !nodeInfo.srs().empty())
    {
        vec2 fl = vecFromUblas<vec2>(nodeInfo.extents().ll);
        vec2 fu = vecFromUblas<vec2>(nodeInfo.extents().ur);
        vec3 el = vec2to3(fl, double(meta.geomExtents.z.min));
        vec3 eu = vec2to3(fu, double(meta.geomExtents.z.max));
        vec3 ed = eu - el;
        vec3 *corners = node.cornersPhys;
        for (uint32 i = 0; i < 8; i++)
        {
            vec3 f = lowerUpperCombine(i).cwiseProduct(ed) + el;
            f = cnv->convert(f,
                    nodeInfo.node(), Srs::Physical);
            corners[i] = f;
        }

        // obb
        if (id.lod > 4)
        {
            vec3 center = vec3(0,0,0);
            for (uint32 i = 0; i < 8; i++)
                center += corners[i];
            center /= 8;

            vec3 f = corners[4] - corners[0];
            vec3 u = corners[2] - corners[0];
            mat4 t = lookAt(center, center + f, u);

            MetaNode::Obb obb;
            obb.rotInv = t.inverse();
            static const double di = std::numeric_limits<double>::infinity();
            vec3 vi(di, di, di);
            obb.points[0] = vi;
            obb.points[1] = -vi;

            for (uint32 i = 0; i < 8; i++)
            {
                vec3 p = vec4to3(vec4(t * vec3to4(corners[i], 1)), false);
                obb.points[0] = min(obb.points[0], p);
                obb.points[1] = max(obb.points[1], p);
            }

            node.obb = obb;
        }

        // disks
        if (id.lod > 4)
        {
            vec2 sds2 = vec2((fu + fl) * 0.5);
            vec3 sds = vec2to3(sds2, double(meta.geomExtents.z.min));
            vec3 vn1 = cnv->convert(sds,
                nodeInfo.node(), Srs::Physical);
            node.diskNormalPhys = vn1.normalized();
            node.diskHeightsPhys[0] = vn1.norm();
            sds = vec2to3(sds2, double(meta.geomExtents.z.max));
            vec3 vn2 = cnv->convert(sds,
                nodeInfo.node(), Srs::Physical);
            node.diskHeightsPhys[1] = vn2.norm();
            sds = vec2to3(fu, double(meta.geomExtents.z.min));
            vec3 vc = cnv->convert(sds,
                nodeInfo.node(), Srs::Physical);
            node.diskHalfAngle = std::acos(
                dot(node.diskNormalPhys, vc.normalized()));
        }
    }
    else if (meta.extents.ll != meta.extents.ur)
    {
        vec3 fl = vecFromUblas<vec3>(meta.extents.ll);
        vec3 fu = vecFromUblas<vec3>(meta.extents.ur);
        vec3 fd = fu - fl;
        vec3 el = vecFromUblas<vec3>
                (m->referenceFrame.division.extents.ll);
        vec3 eu = vecFromUblas<vec3>
                (m->referenceFrame.division.extents.ur);
        vec3 ed = eu - el;
        for (uint32 i = 0; i < 8; i++)
        {
            vec3 f = lowerUpperCombine(i).cwiseProduct(fd) + fl;
            node.cornersPhys[i] = f.cwiseProduct(ed) + el;
        }
    }
    else
    {
        LOG(warn2) << "Tile <" << id
            << "> does not have neither extents nor geomExtents";
    }

    // aabb
    if (id.lod > 2)
    {
        node.aabbPhys[0] = node.aabbPhys[1] = node.cornersPhys[0];
        for (const vec3 &it : node.cornersPhys)
        {
            node.aabbPhys[0] = min(node.aabbPhys[0], it);
            node.aabbPhys[1] = max(node.aabbPhys[1], it);
        }
    }

    // surrogate
    if (vtslibs::vts::GeomExtents::validSurrogate(
                meta.geomExtents.surrogate))
    {
        vec2 fl = vecFromUblas<vec2>(nodeInfo.extents().ll);
        vec2 fu = vecFromUblas<vec2>(nodeInfo.extents().ur);
        vec3 sds = vec2to3(vec2((fl + fu) * 0.5),
                           double(meta.geomExtents.surrogate));
        node.surrogatePhys = cnv->convert(sds,
                            nodeInfo.node(), Srs::Physical);
        node.surrogateNav = cnv->convert(sds,
                            nodeInfo.node(), Srs::Navigation)[2];
    }

    // texelSize
    {
        bool applyTexelSize = meta.flags()
            & vtslibs::vts::MetaNode::Flag::applyTexelSize;
        bool applyDisplaySize = meta.flags()
            & vtslibs::vts::MetaNode::Flag::applyDisplaySize;

        if (applyTexelSize)
        {
            node.texelSize = meta.texelSize;
        }
        else if (applyDisplaySize)
        {
            vec3 s = node.aabbPhys[1] - node.aabbPhys[0];
            double m = std::max(s[0], std::max(s[1], s[2]));
            node.texelSize = m / meta.displaySize;
        }
        else
        {
            // the test fails by default
            node.texelSize = std::numeric_limits<double>::infinity();
        }
    }

    return node;
}

void MetaTile::decode()
{
    std::shared_ptr<Mapconfig> m = mapconfig.lock();
    if (!m)
    {
        throw std::runtime_error("Decoding metatile after the "
            "corresponding mapconfig has expired");
    }

    // decode the whole tile
    {
        detail::BufferStream w(fetch->reply.content);
        *(vtslibs::vts::MetaTile*)this = vtslibs::vts::loadMetaTile(w,
                m->referenceFrame.metaBinaryOrder, name);
    }

    // precompute metanodes
    metas.resize(size_ * size_);
    vtslibs::vts::MetaTile::for_each([&](const vtslibs::vts::TileId &id,
        vtslibs::vts::MetaNode &node) {
            node.displaySize = 1024; // forced override
            if (node.flags() == 0)
                return;
            metas[(id.y - origin_.y) * size_ + id.x - origin_.x]
                = generateMetaNode(m, id, node);
        });

    info.ramMemoryCost += sizeof(*this);
    info.ramMemoryCost += size_ * size_
        * (sizeof(vtslibs::vts::MetaNode) + sizeof(MetaNode));
}

FetchTask::ResourceType MetaTile::resourceType() const
{
    return FetchTask::ResourceType::MetaTile;
}

std::shared_ptr<const MetaNode> MetaTile::getNode(const TileId &tileId) const
{
    const MetaNode *n = &*metas[index(tileId, false)];
    return std::shared_ptr<const MetaNode>(shared_from_this(), n);
}

} // namespace vts
