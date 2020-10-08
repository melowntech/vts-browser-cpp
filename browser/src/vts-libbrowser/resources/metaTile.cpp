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

#include <dbglog/dbglog.hpp>

#include <optick.h>

namespace vts
{

using vtslibs::vts::NodeInfo;

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

MetaNode::MetaNode() :
    diskNormalPhys(nan3()),
    diskHeightsPhys(nan2()),
    diskHalfAngle(nan1()),
    texelSize(inf1())
{
    // initialize aabb to universe
    aabbPhys[0] = -inf3();
    aabbPhys[1] = inf3();
}

vec3 MetaNode::cornersPhys(uint32 index) const
{
    assert(index < 8);
    return lowerUpperCombine(index).cwiseProduct(aabbPhys[1] - aabbPhys[0]) + aabbPhys[0];
}

MetaTile::MetaTile(vts::MapImpl *map, const std::string &name) :
    Resource(map, name),
    vtslibs::vts::MetaTile(vtslibs::vts::TileId(), 0)
{
    mapconfig = map->mapconfig;
}

Extents2 subExtents(const Extents2 &parentExtents,
    const TileId& parentId, const TileId &targetId)
{
    const TileId lid(vtslibs::vts::local(parentId.lod, targetId));
    const std::size_t tc(vtslibs::vts::tileCount(lid.lod));
    const math::Size2f rs(math::size(parentExtents));
    const math::Size2f ts(rs.width / tc, rs.height / tc);
    return  math::Extents2(
        parentExtents.ll(0) + lid.x * ts.width,
        parentExtents.ur(1) - (lid.y + 1) * ts.height,
        parentExtents.ll(0) + (lid.x + 1) * ts.width,
        parentExtents.ur(1) - lid.y * ts.height);
}

MetaNode generateMetaNode(const std::shared_ptr<Mapconfig> &m,
    const TileId &id, const vtslibs::vts::MetaNode &meta)
{
    return generateMetaNode(m, m->map->convertor, id, meta);
}

MetaNode generateMetaNode(const std::shared_ptr<Mapconfig> &m,
    const std::shared_ptr<CoordManip> &cnv,
    const vtslibs::vts::TileId &id, const vtslibs::vts::MetaNode &meta)
{
    const bool projected = m->navigationSrsType() == vtslibs::registry::Srs::Type::projected;

    MetaNode node;
    std::string srs;
    {
        TileId t = id;
        while (true)
        {
            bool found = false;
            for (const auto &d : m->referenceDivisionNodeInfos)
            {
                if (d.nodeId() != t)
                    continue;
                node.tileId = id;
                node.localId = vtslibs::vts::local(d.nodeId().lod, id);
                node.extents = subExtents(d.extents(), d.nodeId(), id);
                srs = d.node().srs;
                found = true;
            }
            if (found)
                break;
            assert(t.lod > 0);
            t = vtslibs::vts::parent(t);
        }
        assert(node.tileId == id);
    }

    // corners
    vec3 cornersPhys[8]; // oriented trapezoid bounding box corners
    if (!vtslibs::vts::empty(meta.geomExtents) && !srs.empty())
    {
        vec2 fl = vecFromUblas<vec2>(node.extents.ll);
        vec2 fu = vecFromUblas<vec2>(node.extents.ur);
        vec3 el = vec2to3(fl, double(meta.geomExtents.z.min));
        vec3 eu = vec2to3(fu, double(meta.geomExtents.z.min));
        vec3 ed = eu - el;
        for (uint32 i = 0; i < 4; i++)
        {
            vec3 f = lowerUpperCombine(i).cwiseProduct(ed) + el;
            f = cnv->convert(f, srs, Srs::Physical);
            cornersPhys[i] = f;
        }
        for (uint32 i = 4; i < 8; i++)
        {
            vec3 bottom = cornersPhys[i - 4];
            vec3 up = projected ? vec3(0, 0, 1) : bottom.normalized();
            cornersPhys[i] = bottom + up * (double(meta.geomExtents.z.max) - double(meta.geomExtents.z.min));
        }

        // disks
        if (id.lod > 4 && !projected)
        {
            vec2 sds2 = vec2((fu + fl) * 0.5);
            vec3 sds = vec2to3(sds2, double(meta.geomExtents.z.min));
            vec3 vn1 = cnv->convert(sds, srs, Srs::Physical);
            node.diskNormalPhys = vn1.normalized();
            node.diskHeightsPhys[0] = vn1.norm();
            node.diskHeightsPhys[1] = node.diskHeightsPhys[0] + double(meta.geomExtents.z.max) - double(meta.geomExtents.z.min);
            sds = vec2to3(fu, double(meta.geomExtents.z.min));
            vec3 vc = cnv->convert(sds, srs, Srs::Physical);
            node.diskHalfAngle = std::acos(dot(node.diskNormalPhys, vc.normalized()));
        }
    }
    else if (meta.extents.ll != meta.extents.ur)
    {
        vec3 fl = vecFromUblas<vec3>(meta.extents.ll);
        vec3 fu = vecFromUblas<vec3>(meta.extents.ur);
        vec3 fd = fu - fl;
        vec3 el = vecFromUblas<vec3>(m->referenceFrame.division.extents.ll);
        vec3 eu = vecFromUblas<vec3>(m->referenceFrame.division.extents.ur);
        vec3 ed = eu - el;
        for (uint32 i = 0; i < 8; i++)
        {
            vec3 f = lowerUpperCombine(i).cwiseProduct(fd) + fl;
            cornersPhys[i] = f.cwiseProduct(ed) + el;
        }
    }
    else
    {
        for (vec3 &c : cornersPhys)
            c = nan3();
        LOG(warn1) << "Tile <" << id << "> does not have neither extents nor geomExtents";
    }

    // obb
    if (!std::isnan(cornersPhys[0][0]) && id.lod > 4)
    {
        vec3 center = vec3(0,0,0);
        for (uint32 i = 0; i < 8; i++)
            center += cornersPhys[i];
        center /= 8;

        const auto &cp = cornersPhys;
        vec3 f = cp[2] + cp[3] + cp[6] + cp[7] - cp[0] - cp[1] - cp[5] - cp[4];
        vec3 u = cp[4] + cp[5] + cp[6] + cp[7] - cp[0] - cp[1] - cp[2] - cp[3];
        mat4 t = lookAt(center, center + f, u);

        MetaNode::Obb obb;
        obb.rotInv = t.inverse();
        obb.points[0] = inf3();
        obb.points[1] = -inf3();

        for (uint32 i = 0; i < 8; i++)
        {
            vec3 p = vec4to3(vec4(t * vec3to4(cornersPhys[i], 1)), false);
            obb.points[0] = min(obb.points[0], p);
            obb.points[1] = max(obb.points[1], p);
        }

        node.obb = obb;
    }

    // aabb
    if (!std::isnan(cornersPhys[0][0]) && id.lod > 2)
    {
        node.aabbPhys[0] = node.aabbPhys[1] = cornersPhys[0];
        for (const vec3 &it : cornersPhys)
        {
            node.aabbPhys[0] = min(node.aabbPhys[0], it);
            node.aabbPhys[1] = max(node.aabbPhys[1], it);
        }
    }

    // surrogate
    if (vtslibs::vts::GeomExtents::validSurrogate(meta.geomExtents.surrogate))
    {
        vec2 fl = vecFromUblas<vec2>(node.extents.ll);
        vec2 fu = vecFromUblas<vec2>(node.extents.ur);
        vec3 sds = vec2to3(vec2((fl + fu) * 0.5), double(meta.geomExtents.surrogate));
        node.surrogatePhys = cnv->convert(sds, srs, Srs::Physical);
        node.surrogateNav = cnv->convert(sds, srs, Srs::Navigation)[2];
    }

    // texelSize
    if (node.aabbPhys[1][0] != inf1())
    {
        if (meta.flags() & vtslibs::vts::MetaNode::Flag::applyTexelSize)
        {
            node.texelSize = meta.texelSize;
        }
        else if (meta.flags() & vtslibs::vts::MetaNode::Flag::applyDisplaySize)
        {
            vec3 s = node.aabbPhys[1] - node.aabbPhys[0];
            double m = std::max(s[0], std::max(s[1], s[2]));
            node.texelSize = m / meta.displaySize;
        }
    }

    return node;
}

void MetaTile::decode()
{
    OPTICK_EVENT("decode meta tile");

    std::shared_ptr<Mapconfig> m = mapconfig.lock();
    if (!m)
    {
        LOGTHROW(err2, std::runtime_error) << "Decoding metatile after the corresponding mapconfig has expired";
    }

    // decode the whole tile
    {
        detail::BufferStream w(fetch->reply.content);
        *(vtslibs::vts::MetaTile*)this = vtslibs::vts::loadMetaTile(w, m->referenceFrame.metaBinaryOrder, name);
    }

    // precompute metanodes
    metas.resize(size_ * size_);
    vtslibs::vts::MetaTile::for_each([&](const vtslibs::vts::TileId &id, vtslibs::vts::MetaNode &node)
        {
            if (node.flags() == 0)
                return;
            node.displaySize = 1024; // forced override
            metas[(id.y - origin_.y) * size_ + id.x - origin_.x] = generateMetaNode(m, m->convertorData, id, node);
        });

    info.ramMemoryCost += sizeof(*this);
    info.ramMemoryCost += size_ * size_ * (sizeof(vtslibs::vts::MetaNode) + sizeof(MetaNode));
}

FetchTask::ResourceType MetaTile::resourceType() const
{
    return FetchTask::ResourceType::MetaTile;
}

std::shared_ptr<const MetaNode> MetaTile::getNode(const TileId &tileId)
{
    const auto idx = index(tileId, false);
    const boost::optional<MetaNode> &mn = metas[idx];
    assert(mn);
    return std::shared_ptr<const MetaNode>(shared_from_this(), mn.get_ptr());
}

} // namespace vts
