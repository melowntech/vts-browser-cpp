#include "map.hpp"
#include <boost/algorithm/string.hpp>

namespace vts
{

MapImpl::Navigation::Navigation() :
    inertiaMotion(0,0,0), inertiaRotation(0,0,0),
    inertiaViewExtent(0)
{}

void MapImpl::rotate(const vec3 &value)
{
    if (!mapConfig || !*mapConfig)
        return;
    
    vtslibs::registry::Position &pos = mapConfig->position;
    vec3 rot(value[0] * -0.2, value[1] * -0.1, 0);
    switch (mapConfig->srs.get
            (mapConfig->referenceFrame.model.navigationSrs).type)
    {
    case vtslibs::registry::Srs::Type::projected:
        break; // do nothing
    case vtslibs::registry::Srs::Type::geographic:
        rot(0) *= -1;
        break;
    default:
        throw std::invalid_argument("not implemented navigation srs type");
    }
    navigation.inertiaRotation += rot;
}

void MapImpl::pan(const vec3 &value)
{
    if (!mapConfig || !*mapConfig)
        return;
    
    vtslibs::registry::Position &pos = mapConfig->position;
    switch (mapConfig->srs.get
            (mapConfig->referenceFrame.model.navigationSrs).type)
    {
    case vtslibs::registry::Srs::Type::projected:
    {
        mat3 rot = upperLeftSubMatrix
                (rotationMatrix(2, degToRad(pos.orientation(0))));
        vec3 move = vec3(-value[0], value[1], 0);
        move = rot * move * (pos.verticalExtent / 800);
        navigation.inertiaMotion += move;
    } break;
    case vtslibs::registry::Srs::Type::geographic:
    {
        mat3 rot = upperLeftSubMatrix
                (rotationMatrix(2, degToRad(-pos.orientation(0))));
        vec3 move = vec3(-value[0], value[1], 0);
        move = rot * move * (pos.verticalExtent / 800);
        vec3 p = vecFromUblas<vec3>(pos.position);
        p = mapConfig->convertor->navGeodesicDirect(p, 90, move(0));
        p = mapConfig->convertor->navGeodesicDirect(p, 0, move(1));
        p -= vecFromUblas<vec3>(pos.position);
        p(0) = modulo(p(0) + 180, 360) - 180;
        navigation.inertiaMotion += p;
    } break;
    default:
        throw std::invalid_argument("not implemented navigation srs type");
    }
    double cur = pos.verticalExtent + navigation.inertiaViewExtent;
    navigation.inertiaViewExtent += cur * pow(1.001, -value[2]) - cur;
    
    { // vertical camera adjustment
        auto h = std::make_shared<HeightRequest>(vec3to2(
                    vecFromUblas<vec3>(mapConfig->position.position)));
        if (navigation.panZQueue.size() < 2)
            navigation.panZQueue.push(h);
        else
            navigation.panZQueue.back() = h;
    }
}

HeightRequest::HeightRequest(const vec2 &navPos) :
    navPos(navPos),
    sds(std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN()),
    interpol(std::numeric_limits<double>::quiet_NaN(),
             std::numeric_limits<double>::quiet_NaN())
{}

Validity HeightRequest::process(MapImpl *map)
{
    if (result)
        return Validity::Valid;
    
    if (corners.empty())
    {
        // find initial position
        corners.reserve(4);
        auto nisds = map->findInfoNavRoot(navPos);
        sds = nisds.second;
        nodeInfo.emplace(map->findInfoSdsSampled(nisds.first, sds));
        
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
            default:
                assert(false);
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
    height = map->mapConfig->convertor->convert(vec2to3(sds, height),
                        nodeInfo->srs(),
                        map->mapConfig->referenceFrame.model.navigationSrs)(2);
    result.emplace(height);
    return Validity::Valid;
}

HeightRequest::CornerRequest::CornerRequest(const NodeInfo &nodeInfo) :
    nodeInfo(nodeInfo)
{}

Validity HeightRequest::CornerRequest::process(MapImpl *map)
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
    default:
        assert(false);
    }
    
    // check id
    if (trav->nodeInfo.nodeId() == nodeInfo.nodeId() || trav->childs.empty())
    {
        result.emplace(trav->metaNode.geomExtents.surrogate);
        return process(nullptr);
    }
    
    { // find child
        uint32 lodDiff = nodeInfo.nodeId().lod-trav->nodeInfo.nodeId().lod-1;
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
    default:
        assert(false);
    }
    
    // apply the height to the camera
    assert(nh == nh);
    double h = mapConfig->position.position[2];
    if (navigation.lastPanZShift)
        navigation.inertiaMotion(2) += nh - *navigation.lastPanZShift;
    else
        mapConfig->position.position[2] = nh;
    navigation.lastPanZShift.emplace(nh);
    navigation.panZQueue.pop();
}

const std::pair<NodeInfo, vec2> MapImpl::findInfoNavRoot(const vec2 &navPos)
{
    for (auto &&it : mapConfig->referenceFrame.division.nodes)
    {
        if (it.second.partitioning.mode
                != vtslibs::registry::PartitioningMode::bisection)
            continue;
        NodeInfo ni(mapConfig->referenceFrame, it.first,
                    false, *mapConfig);
        vec2 sds = vec3to2(mapConfig->convertor->convert(vec2to3(navPos, 0),
            mapConfig->referenceFrame.model.navigationSrs, it.second.srs));
        if (!ni.inside(vecToUblas<math::Point2>(sds)))
            continue;
        return std::make_pair(ni, sds);
    }
    assert(false);
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
    assert(false);
}

} // namespace vts
