#include "map.h"

namespace melown
{

const NodeInfo HeightRequest::findPosition(
        NodeInfo &info, const vec2 &pos, double viewExtent)
{
    double desire = std::log2(4 * info.extents().size()
                              / viewExtent);// - 8;
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

HeightRequest::HeightRequest(MapImpl *map) :
    frameIndex(map->statistics.frameIndex)
{
    vec3 navPos = vecFromUblas<vec3>(map->mapConfig->position.position);
    for (auto &&it : map->mapConfig->referenceFrame.division.nodes)
    {
        if (it.second.partitioning.mode
                != vtslibs::registry::PartitioningMode::bisection)
            continue;
        NodeInfo ni(map->mapConfig->referenceFrame, it.first,
                    false, *map->mapConfig);
        vec2 sdp = vec3to2(map->mapConfig->convertor->convert(navPos,
            map->mapConfig->referenceFrame.model.navigationSrs, it.second.srs));
        if (!ni.inside(vecToUblas<math::Point2>(sdp)))
            continue;
        nodeInfo = findPosition(ni, sdp,
            map->mapConfig->position.verticalExtent);
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
    pos.orientation += vecToUblas<math::Point3>(rot);
    
    //LOG(info3) << "position: " << mapConfig->position.position;
    //LOG(info3) << "rotation: " << mapConfig->position.orientation;
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
        pos.position += vecToUblas<math::Point3>(move);
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
        pos.position = vecToUblas<math::Point3>(p);
        pos.position(0) = modulo(pos.position(0) + 180, 360) - 180;
        // todo - temporarily clamp
        pos.position(1) = std::min(std::max(pos.position(1), -80.0), 80.0);
    } break;
    default:
        throw std::invalid_argument("not implemented navigation srs type");
    }
    pos.verticalExtent *= pow(1.001, -value[2]);
    
    {
        auto h = std::make_shared<HeightRequest>(this);
        if (renderer.panZQueue.size() < 2)
            renderer.panZQueue.push(h);
        else
            renderer.panZQueue.back() = h;
    }
    
    //LOG(info3) << "position: " << mapConfig->position.position;
    //LOG(info3) << "rotation: " << mapConfig->position.orientation;
}

void MapImpl::checkPanZQueue()
{
    if (renderer.panZQueue.empty())
        return;
    HeightRequest &task = *renderer.panZQueue.front();
    
    // find urls to the resources
    const TileId nodeId = task.nodeInfo->nodeId();
    const TileId tileId = roundId(nodeId);
    UrlTemplate::Vars tileVars(tileId);
    const MetaNode *men = nullptr;
    for (auto &&it : mapConfig->surfaceStack)
    {
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
        men = n;
        break;
    }
    if (!men)
    { // this HeightRequest cannot be served
        renderer.panZQueue.pop();
        return;
    }
    
    // acquire the height
    vec2 exU = vecFromUblas<vec2>(task.nodeInfo->extents().ur);
    vec2 exL = vecFromUblas<vec2>(task.nodeInfo->extents().ll);
    vec2 exS = (exU + exL) * 0.5;
    double nh = mapConfig->convertor->convert(vec2to3(exS,
                        men->geomExtents.surrogate),
                        task.nodeInfo->srs(),
                        mapConfig->referenceFrame.model.navigationSrs)(2);
    
    //LOG(info3) << "id: " << task.nodeInfo->nodeId() << " height: " << nh;
    
    // apply the height to the camera
    double h = mapConfig->position.position[2];
    if (renderer.lastPanZShift)
        h += nh - *renderer.lastPanZShift;
    else
        h = nh;
    renderer.lastPanZShift.emplace(nh);
    mapConfig->position.position[2] = h;
    renderer.panZQueue.pop();
}

} // namespace melown
