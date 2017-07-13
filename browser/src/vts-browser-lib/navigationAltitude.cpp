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

/*
class HeightRequest
{
public:
    struct CornerRequest
    {
        const NodeInfo nodeInfo;
        std::shared_ptr<TraverseNode> trav;
        boost::optional<double> result;
        
        CornerRequest(const NodeInfo &nodeInfo) :
            nodeInfo(nodeInfo)
        {}
        
        Validity process(class MapImpl *map)
        {
            if (result)
                return vtslibs::vts::GeomExtents::validSurrogate(*result)
                        ? Validity::Valid : Validity::Invalid;
            
            if (!trav)
            {
                trav = map->renderer.traverseRoot;
                if (!trav)
                    return Validity::Indeterminate;
            }
            
            // load if needed
            switch (trav->validity)
            {
            case Validity::Invalid:
                return Validity::Invalid;
            case Validity::Indeterminate:
                map->traverse(trav, true);
                return Validity::Indeterminate;
            case Validity::Valid:
                break;
            }
            
            // check id
            if (trav->nodeInfo.nodeId() == nodeInfo.nodeId()
                    || trav->childs.empty())
            {
                result = trav->surrogateValue;
                return process(nullptr);
            }
            
            { // find child
                uint32 lodDiff = nodeInfo.nodeId().lod
                            - trav->nodeInfo.nodeId().lod - 1;
                TileId id = nodeInfo.nodeId();
                id.lod -= lodDiff;
                id.x >>= lodDiff;
                id.y >>= lodDiff;
                
                for (auto &&it : trav->childs)
                {
                    if (it->nodeInfo.nodeId() == id)
                    {
                        trav = it;
                        return process(map);
                    }
                }    
            }
            return Validity::Invalid;
        }
    };
    
    std::vector<CornerRequest> corners;
    
    boost::optional<NodeInfo> nodeInfo;
    boost::optional<double> result;
    const vec2 navPos;
    vec2 sds;
    vec2 interpol;
    double resetOffset;
    
    HeightRequest(const vec2 &navPos) : navPos(navPos),
        sds(std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::quiet_NaN()),
        interpol(std::numeric_limits<double>::quiet_NaN(),
                 std::numeric_limits<double>::quiet_NaN()),
        resetOffset(std::numeric_limits<double>::quiet_NaN())
    {}
    
    Validity process(class MapImpl *map)
    {
        if (result)
            return Validity::Valid;
        
        if (corners.empty())
        {
            // find initial position
            try
            {
                corners.reserve(4);
                auto nisds = map->findInfoNavRoot(navPos);
                sds = nisds.second;
                nodeInfo = boost::in_place
                    (map->findInfoSdsSampled(nisds.first, sds));
            }
            catch (const std::runtime_error &)
            {
                return Validity::Invalid;
            }
            
            // find top-left corner position
            math::Extents2 ext = nodeInfo->extents();
            vec2 center = vecFromUblas<vec2>(ext.ll + ext.ur) * 0.5;
            vec2 size = vecFromUblas<vec2>(ext.ur - ext.ll);
            interpol = sds - center;
            interpol(0) /= size(0);
            interpol(1) /= size(1);
            TileId cornerId = nodeInfo->nodeId();
            if (sds(0) < center(0))
            {
                cornerId.x--;
                interpol(0) += 1;
            }
            if (sds(1) < center(1))
                interpol(1) += 1;
            else
                cornerId.y--;
            
            // prepare all four corners
            for (uint32 i = 0; i < 4; i++)
            {
                TileId nodeId = cornerId;
                nodeId.x += i % 2;
                nodeId.y += i / 2;
                corners.emplace_back(NodeInfo(map->mapConfig->referenceFrame,
                                              nodeId, false, *map->mapConfig));
            }
            
            map->statistics.lastHeightRequestLod = nodeInfo->nodeId().lod;
        }
        
        { // process corners
            assert(corners.size() == 4);
            bool determined = true;
            for (auto &&it : corners)
            {
                switch (it.process(map))
                {
                case Validity::Invalid:
                    return Validity::Invalid;
                case Validity::Indeterminate:
                    determined = false;
                    break;
                case Validity::Valid:
                    break;
                }
            }
            if (!determined)
                return Validity::Indeterminate; // try again later
        }
        
        // find the height
        assert(interpol(0) >= 0 && interpol(0) <= 1);
        assert(interpol(1) >= 0 && interpol(1) <= 1);
        double height = interpolate(
            interpolate(*corners[2].result, *corners[3].result, interpol(0)),
            interpolate(*corners[0].result, *corners[1].result, interpol(0)),
            interpol(1));
        height = map->convertor->convert(vec2to3(sds, height),
            nodeInfo->srs(),
            map->mapConfig->referenceFrame.model.navigationSrs)(2);
        result = height;
        return Validity::Valid;
    }
};

void MapImpl::checkPanZQueue()
{
    if (navigation.panZQueue.empty())
        return;

    HeightRequest &task = *navigation.panZQueue.front();
    double nh = std::numeric_limits<double>::quiet_NaN();
    switch (task.process(this))
    {
    case Validity::Indeterminate:
        return; // try again later
    case Validity::Invalid:
        navigation.panZQueue.pop();
        return; // request cannot be served
    case Validity::Valid:
        nh = *task.result;
        break;
    }

    // apply the height to the camera
    assert(nh == nh);
    if (task.resetOffset == task.resetOffset)
        navigation.targetPoint(2) = nh + task.resetOffset;
    else if (navigation.lastPanZShift)
        navigation.targetPoint(2) += nh - *navigation.lastPanZShift;
    navigation.lastPanZShift = nh;
    navigation.panZQueue.pop();
}

const std::pair<NodeInfo, vec2> MapImpl::findInfoNavRoot(const vec2 &navPos)
{
    for (auto &&it : mapConfig->referenceFrame.division.nodes)
    {
        if (it.second.partitioning.mode
                != vtslibs::registry::PartitioningMode::bisection)
            continue;
        NodeInfo ni(mapConfig->referenceFrame, it.first, false, *mapConfig);
        try
        {
            vec2 sds = vec3to2(convertor->convert(vec2to3(navPos, 0),
                mapConfig->referenceFrame.model.navigationSrs, it.second.srs));
            if (!ni.inside(vecToUblas<math::Point2>(sds)))
                continue;
            return std::make_pair(ni, sds);
        }
        catch(const std::exception &)
        {
            // do nothing
        }
    }
    LOGTHROW(err1, std::runtime_error) << "Invalid position";
    throw; // shut up compiler warning
}

const NodeInfo MapImpl::findInfoSdsSampled(const NodeInfo &info,
                                           const vec2 &sdsPos)
{
    double desire = std::log2(options.navigationSamplesPerViewExtent
            * info.extents().size() / mapConfig->position.verticalExtent);
    if (desire < 3)
        return info;
    
    vtslibs::vts::Children childs = vtslibs::vts::children(info.nodeId());
    for (uint32 i = 0; i < childs.size(); i++)
    {
        NodeInfo ni = info.child(childs[i]);
        if (!ni.inside(vecToUblas<math::Point2>(sdsPos)))
            continue;
        return findInfoSdsSampled(ni, sdsPos);
    }
    LOGTHROW(err1, std::runtime_error) << "Invalid position";
    throw; // shut up compiler warning
}
*/

void MapImpl::updatePositionAltitudeShift()
{
    assert(renderer.traverseRoot);
    assert(convertor);

    statistics.desiredNavigationLod = 0;
    statistics.usedNavigationlod = 0;
    if (!renderer.traverseRoot || !renderer.traverseRoot->meta)
        return;

    // find surface division coordinates (and appropriate node info)
    vec2 sds;
    boost::optional<NodeInfo> info;
    for (auto &&it : mapConfig->referenceFrame.division.nodes)
    {
        if (it.second.partitioning.mode
                != vtslibs::registry::PartitioningMode::bisection)
            continue;
        NodeInfo ni(mapConfig->referenceFrame, it.first, false, *mapConfig);
        try
        {
            sds = vec3to2(convertor->convert(navigation.targetPoint,
                mapConfig->referenceFrame.model.navigationSrs, it.second.srs));
            if (!ni.inside(vecToUblas<math::Point2>(sds)))
                continue;
            info = ni;
            break;
        }
        catch(const std::exception &)
        {
            // do nothing
        }
    }
    if (!info)
        return;

    // desired navigation lod
    uint32 desiredLod = -std::log2(mapConfig->position.verticalExtent
                                  / info->extents().size()
                                  / options.navigationSamplesPerViewExtent);
    desiredLod = std::max(desiredLod, 0u);
    statistics.desiredNavigationLod = desiredLod;

    // traverse up to the desired navigation lod
    std::shared_ptr<TraverseNode> t = renderer.traverseRoot;
    while (t->nodeInfo.nodeId().lod < desiredLod)
    {
        auto p = t;
        for (auto &&it : t->childs)
        {
            if (it->meta)
            {
                if (!it->nodeInfo.inside(vecToUblas<math::Point2>(sds)))
                    continue;
                t = it;
                break;
            }
        }
        if (t == p)
            break;
    }
    if (!vtslibs::vts::GeomExtents::validSurrogate(
				t->meta->geomExtents.surrogate))
        return;
    assert(info->inside(vecToUblas<math::Point2>(sds)));
    assert(t->meta);
    statistics.usedNavigationlod = t->nodeInfo.nodeId().lod;
    
    // todo interpolation

    // set the altitude
    //if (navigation.positionAltitudeResetHeight)
    //{
    //    navigation.targetPoint[2] = t->surrogateValue
    //            + *navigation.positionAltitudeResetHeight;
    //    navigation.positionAltitudeResetHeight.reset();
    //}
    //else
    if (navigation.lastPositionAltitudeShift)
    {
        navigation.targetPoint[2] += t->meta->geomExtents.surrogate
                - *navigation.lastPositionAltitudeShift;
    }
    else
    {
        navigation.targetPoint[2] = t->meta->geomExtents.surrogate;
    }
    navigation.lastPositionAltitudeShift = t->meta->geomExtents.surrogate;
}

void MapImpl::resetPositionAltitude(double altitude)
{
    if (altitude == altitude)
        navigation.positionAltitudeResetHeight = altitude;
    else
        navigation.positionAltitudeResetHeight.reset();
    navigation.lastPositionAltitudeShift.reset();
}

} // namespace vts
