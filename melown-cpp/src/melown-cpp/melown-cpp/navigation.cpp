#include "map.hpp"
#include <boost/algorithm/string.hpp>

namespace melown
{

const NodeInfo HeightRequest::findPosition(
        NodeInfo &info, const vec2 &pos, MapImpl *map)
{
    double desire = std::log2(map->options.navigationSamplesPerViewExtent
            * info.extents().size() / map->mapConfig->position.verticalExtent);
    if (desire < 3)
        return info;
    
    vtslibs::vts::Children childs = vtslibs::vts::children(info.nodeId());
    for (uint32 i = 0; i < childs.size(); i++)
    {
        NodeInfo ni = info.child(childs[i]);
        if (!ni.inside(vecToUblas<math::Point2>(pos)))
            continue;
        return findPosition(ni, pos, map);
    }
    assert(false);
}

HeightRequest::HeightRequest(MapImpl *map) :
    surface(nullptr),
    frameIndex(map->statistics.frameIndex)
{
    vec3 navPos = vecFromUblas<vec3>(map->mapConfig->position.position);
    // find suitable spatial division subtree node
    for (auto &&it : map->mapConfig->referenceFrame.division.nodes)
    {
        if (it.second.partitioning.mode
                != vtslibs::registry::PartitioningMode::bisection)
            continue;
        NodeInfo ni(map->mapConfig->referenceFrame, it.first,
                    false, *map->mapConfig);
        sds = vec3to2(map->mapConfig->convertor->convert(navPos,
            map->mapConfig->referenceFrame.model.navigationSrs, it.second.srs));
        if (!ni.inside(vecToUblas<math::Point2>(sds)))
            continue;
        nodeInfo = findPosition(ni, sds, map);
        map->statistics.lastHeightRequestLod = nodeInfo->nodeId().lod;
        return;
    }
    assert(false);
}

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
    renderer.inertiaRotation += rot;
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
        renderer.inertiaMotion += move;
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
        renderer.inertiaMotion += p - vecFromUblas<vec3>(pos.position);
    } break;
    default:
        throw std::invalid_argument("not implemented navigation srs type");
    }
    double cur = pos.verticalExtent + renderer.inertiaViewExtent;
    renderer.inertiaViewExtent += cur * pow(1.001, -value[2]) - cur;
    
    { // vertical camera adjustment
        auto h = std::make_shared<HeightRequest>(this);
        if (renderer.panZQueue.size() < 2)
            renderer.panZQueue.push(h);
        else
            renderer.panZQueue.back() = h;
    }
}

void MapImpl::checkPanZQueue()
{
    if (renderer.panZQueue.empty())
        return;
    HeightRequest &task = *renderer.panZQueue.front();
    
    // find urls to the resources
    if (!task.surface)
    {
        for (auto &&it : mapConfig->surfaceStack)
        {
            uint32 lodMax = it.surface->lodRange.max;
            TileId nodeId = task.nodeInfo->nodeId();
            boost::optional<NodeInfo> info;
            if (nodeId.lod > lodMax)
            {
                uint32 lodDiff = nodeId.lod - lodMax;
                nodeId.lod -= lodDiff;
                nodeId.x >>= lodDiff;
                nodeId.y >>= lodDiff;
                info.emplace(mapConfig->referenceFrame, nodeId,
                             false, *mapConfig);
            }
            TileId tileId = roundId(nodeId);
            UrlTemplate::Vars tileVars(tileId);
            std::string tileUrl = it.surface->urlMeta(tileVars);
            std::shared_ptr<const MetaTile> t = getMetaTile(tileUrl);
            switch (t->impl->state)
            {
            case ResourceImpl::State::initializing:
            case ResourceImpl::State::downloading:
            case ResourceImpl::State::downloaded:
                return; // try again later
            case ResourceImpl::State::errorDownload:
            case ResourceImpl::State::errorLoad:
                continue;
            case ResourceImpl::State::ready:
                break;
            default:
                assert(false);
            }
            const MetaNode *n = t->get(nodeId, std::nothrow);
            if (!n || n->extents.ll == n->extents.ur
                    || n->alien() != it.alien || !n->geometry())
                continue;
            task.surface = &it;
            if (info)
                task.nodeInfo = info;
            break;
        }
        if (!task.surface)
        {
            renderer.panZQueue.pop();
            return; // this HeightRequest cannot be served
        }
    }
    
    // find interpolation coefficients and corner id
    vec2 center = vecFromUblas<vec2>(task.nodeInfo->extents().ll
                                     + task.nodeInfo->extents().ur) * 0.5;
    vec2 size = vecFromUblas<vec2>(task.nodeInfo->extents().ur
                                     - task.nodeInfo->extents().ll);
    vec2 interpol = task.sds - center;
    interpol(0) /= size(0);
    interpol(1) /= size(1);
    TileId cornerId = task.nodeInfo->nodeId();
    if (task.sds(0) < center(0))
    {
        cornerId.x--;
        interpol(0) += 1;
    }
    if (task.sds(1) < center(1))
        interpol(1) += 1;
    else
        cornerId.y--;
    
    // acguire the four heights
    double heights[4];
    for (uint32 i = 0; i < 4; i++)
    {
        TileId nodeId = cornerId;
        nodeId.x += i % 2;
        nodeId.y += i / 2;
        TileId tileId = roundId(nodeId);
        std::shared_ptr<MetaTile> met = getMetaTile(
                    task.surface->surface->urlMeta(UrlTemplate::Vars(tileId)));
        const MetaNode *men = met->get(nodeId, std::nothrow);
        if (!men || !vtslibs::vts::GeomExtents::validSurrogate(
                    men->geomExtents.surrogate))
        {
            renderer.panZQueue.pop();
            return; // this HeightRequest cannot be served
        }
        heights[i] = men->geomExtents.surrogate;
    }
    
    // interpolate heights
    assert(interpol(0) >= 0 && interpol(0) <= 1);
    assert(interpol(1) >= 0 && interpol(1) <= 1);
    double height = interpolate(
                interpolate(heights[2], heights[3], interpol(0)),
                interpolate(heights[0], heights[1], interpol(0)),
                interpol(1));
    
    // convert height from sds to navigation
    double nh = mapConfig->convertor->convert(vec2to3(task.sds, height),
                        task.nodeInfo->srs(),
                        mapConfig->referenceFrame.model.navigationSrs)(2);
    
    // apply the height to the camera
    assert(nh == nh);
    double h = mapConfig->position.position[2];
    if (renderer.lastPanZShift)
        renderer.inertiaMotion(2) += nh - *renderer.lastPanZShift;
    else
        mapConfig->position.position[2] = nh;
    renderer.lastPanZShift.emplace(nh);
    renderer.panZQueue.pop();
}

} // namespace melown
