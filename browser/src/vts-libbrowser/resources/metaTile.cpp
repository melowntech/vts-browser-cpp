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

//#include <vts-libs/vts/nodeinfo.hpp>
#include <dbglog/dbglog.hpp>

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

NodeInfo makeNodeInfo(const std::shared_ptr<Mapconfig> &m, TileId id)
{
    std::vector<TileId> chain;
    chain.reserve(id.lod + 1);
    while (true)
    {
        for (const auto &d : m->referenceDivisionNodeInfos)
        {
            if (d.nodeId() == id)
            {
                NodeInfo inf = d;
                while (!chain.empty())
                {
                    inf = inf.child(chain.back());
                    chain.pop_back();
                }
                return inf;
            }
        }
        if (id.lod == 0)
            return NodeInfo(m->referenceFrame, id, true, *m);
        chain.push_back(id);
        id = vtslibs::vts::parent(id);
    }
}

} // namespace

MetaNode::MetaNode(const NodeInfo &nodeInfo) :
    nodeInfo(nodeInfo),
    tileId(nodeInfo.nodeId()),
    localId(vtslibs::vts::local(nodeInfo)),
    extents(nodeInfo.extents()),
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
    return lowerUpperCombine(index).cwiseProduct(
        aabbPhys[1] - aabbPhys[0]) + aabbPhys[0];
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
    const auto &cnv = m->map->convertor;
    NodeInfo nodeInfo = makeNodeInfo(m, id);
    assert(nodeInfo.nodeId() == id);
    MetaNode node(nodeInfo);

    // corners
    vec3 cornersPhys[8]; // oriented trapezoid bounding box corners
    if (!vtslibs::vts::empty(meta.geomExtents)
            && !nodeInfo.srs().empty())
    {
        vec2 fl = vecFromUblas<vec2>(nodeInfo.extents().ll);
        vec2 fu = vecFromUblas<vec2>(nodeInfo.extents().ur);
        vec3 el = vec2to3(fl, double(meta.geomExtents.z.min));
        vec3 eu = vec2to3(fu, double(meta.geomExtents.z.max));
        vec3 ed = eu - el;
        for (uint32 i = 0; i < 8; i++)
        {
            vec3 f = lowerUpperCombine(i).cwiseProduct(ed) + el;
            f = cnv->convert(f, nodeInfo.node(), Srs::Physical);
            cornersPhys[i] = f;
        }

        // disks
        if (id.lod > 4)
        {
            vec2 sds2 = vec2((fu + fl) * 0.5);
            vec3 sds = vec2to3(sds2, double(meta.geomExtents.z.min));
            vec3 vn1 = cnv->convert(sds, nodeInfo.node(), Srs::Physical);
            node.diskNormalPhys = vn1.normalized();
            node.diskHeightsPhys[0] = vn1.norm();
            sds = vec2to3(sds2, double(meta.geomExtents.z.max));
            vec3 vn2 = cnv->convert(sds, nodeInfo.node(), Srs::Physical);
            node.diskHeightsPhys[1] = vn2.norm();
            sds = vec2to3(fu, double(meta.geomExtents.z.min));
            vec3 vc = cnv->convert(sds, nodeInfo.node(), Srs::Physical);
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
            cornersPhys[i] = f.cwiseProduct(ed) + el;
        }
    }
    else
    {
        for (vec3 &c : cornersPhys)
            c = nan3();
        LOG(warn2) << "Tile <" << id
            << "> does not have neither extents nor geomExtents";
    }

    // obb
    if (!std::isnan(cornersPhys[0][0]) && id.lod > 4)
    {
        vec3 center = vec3(0,0,0);
        for (uint32 i = 0; i < 8; i++)
            center += cornersPhys[i];
        center /= 8;

        vec3 f = cornersPhys[4] - cornersPhys[0];
        vec3 u = cornersPhys[2] - cornersPhys[0];
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
    if (node.aabbPhys[1][0] != inf1())
    {
        if (meta.flags()
            & vtslibs::vts::MetaNode::Flag::applyTexelSize)
        {
            node.texelSize = meta.texelSize;
        }
        else if (meta.flags()
            & vtslibs::vts::MetaNode::Flag::applyDisplaySize)
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
    std::shared_ptr<Mapconfig> m = mapconfig.lock();
    if (!m)
    {
        LOGTHROW(err2, std::runtime_error) << "Decoding metatile after the "
            "corresponding mapconfig has expired";
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
            /*
            if (node.flags() == 0)
                return;
            metas[(id.y - origin_.y) * size_ + id.x - origin_.x]
                = generateMetaNode(m, id, node);
            */
        });

    info.ramMemoryCost += sizeof(*this);
    info.ramMemoryCost += size_ * size_
        * (sizeof(vtslibs::vts::MetaNode) + sizeof(MetaNode));
}

FetchTask::ResourceType MetaTile::resourceType() const
{
    return FetchTask::ResourceType::MetaTile;
}

std::shared_ptr<const MetaNode> MetaTile::getNode(const TileId &tileId)
{
    const auto idx = index(tileId, false);
    boost::optional<MetaNode> &mn = metas[idx];
    if (!mn)
    {
        assert(this->grid_[idx].flags() != 0);
        mn.emplace(generateMetaNode(
            map->mapconfig, tileId, this->grid_[idx]));
    }
    return std::shared_ptr<const MetaNode>(shared_from_this(), mn.get_ptr());
}

} // namespace vts
