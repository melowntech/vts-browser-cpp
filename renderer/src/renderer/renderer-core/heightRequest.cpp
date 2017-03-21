#include "map.h"

namespace melown
{

const vtslibs::vts::NodeInfo HeightRequest::findPosition(
        vtslibs::vts::NodeInfo &info, const vec2 &pos, double viewExtent)
{
    double desire = std::log2(4 * info.extents().size()
                              / viewExtent) - 8;
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

HeightRequest::HeightRequest(const vec2 &navPos, MapImpl *map) :
    frameIndex(map->resources.tickIndex)
{
    for (auto &&it : map->mapConfig->referenceFrame.division.nodes)
    {
        if (!it.second.valid())
            continue;
        NodeInfo ni(map->mapConfig->referenceFrame, it.first,
                    false, *map->mapConfig);
        vec2 sdp = vec3to2(map->convertor->convert(
                       vec2to3(navPos, 0), map->mapConfig
                       ->referenceFrame.model.navigationSrs, it.second.srs));
        if (!ni.inside(vecToUblas<math::Point2>(sdp)))
            continue;
        ni = findPosition(ni, sdp, map->mapConfig->position.verticalExtent);
        pixPos = NavTile::sds2px(sdp, ni.extents());
        nodeId = ni.nodeId();
        return;
    }
    assert(false);
}

} // namespace melown
