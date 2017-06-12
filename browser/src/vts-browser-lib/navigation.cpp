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
#include <boost/algorithm/string.hpp>

namespace vts
{

namespace
{

inline void normalizeAngle(double &a)
{
    a = modulo(a, 360);
}

inline double angularDiff(double a, double b)
{
    assert(a >= 0 && a <= 360);
    assert(b >= 0 && b <= 360);
    double c = b - a;
    if (c > 180)
        c = c - 360;
    else if (c < -180)
        c = c + 360;
    assert(c >= -180 && c <= 180);
    return c;
}

inline vec3 angularDiff(const vec3 &a, const vec3 &b)
{
    return vec3(
        angularDiff(a(0), b(0)),
        angularDiff(a(1), b(1)),
        angularDiff(a(2), b(2))
    );
}

} // namespace

MapImpl::Navigation::Navigation() :
    autoMotion(0,0,0), autoRotation(0,0,0),
    targetPoint(0,0,0), changeRotation(0,0,0),
    targetViewExtent(0), mode(MapOptions::NavigationMode::Azimuthal)
{}

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
            default:
				LOGTHROW(fatal, std::invalid_argument)
						<< "Invalid resource state";
            }
            
            // check id
            if (trav->nodeInfo.nodeId() == nodeInfo.nodeId()
                    || trav->childs.empty())
            {
                result.emplace(trav->surrogateValue);
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
        height = map->convertor->convert(vec2to3(sds, height),
            nodeInfo->srs(),
            map->mapConfig->referenceFrame.model.navigationSrs)(2);
        result.emplace(height);
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
    default:
        LOGTHROW(fatal, std::invalid_argument) << "Invalid resource state";
    }

    // apply the height to the camera
    assert(nh == nh);
    if (task.resetOffset == task.resetOffset)
        navigation.targetPoint(2) = nh + task.resetOffset;
    else if (navigation.lastPanZShift)
        navigation.targetPoint(2) += nh - *navigation.lastPanZShift;
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
    LOGTHROW(fatal, std::invalid_argument) << "Impossible position";
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
    LOGTHROW(fatal, std::invalid_argument) << "Impossible position";
    throw; // shut up compiler warning
}

void MapImpl::resetPositionAltitude(double resetOffset)
{
    navigation.targetPoint(2) = navigation.autoMotion(2) = 0;
    navigation.lastPanZShift.reset();
    std::queue<std::shared_ptr<class HeightRequest>>().swap(
                navigation.panZQueue);
    auto r = std::make_shared<HeightRequest>(
                vec3to2(vecFromUblas<vec3>(mapConfig->position.position)));
    r->resetOffset = resetOffset;
    navigation.panZQueue.push(r);
}

void MapImpl::resetNavigationMode()
{
    if (options.navigationMode == MapOptions::NavigationMode::Dynamic)
        navigation.mode = MapOptions::NavigationMode::Azimuthal;
    else
        navigation.mode = options.navigationMode;
}

void MapImpl::resetAuto()
{
    navigation.autoMotion = navigation.autoRotation = vec3(0,0,0);
}

void MapImpl::convertPositionSubjObj()
{
    vtslibs::registry::Position &pos = mapConfig->position;
    vec3 center, dir, up;
    positionToCamera(center, dir, up);
    double dist = positionObjectiveDistance();
    if (pos.type == vtslibs::registry::Position::Type::objective)
        dist *= -1;
    center += dir * dist;
    pos.position = vecToUblas<math::Point3>(convertor->physToNav(center));
}

void MapImpl::positionToCamera(vec3 &center, vec3 &dir, vec3 &up)
{
    vtslibs::registry::Position &pos = mapConfig->position;
    
    // camera-space vectors
    vec3 rot = vecFromUblas<vec3>(pos.orientation);
    center = vecFromUblas<vec3>(pos.position);
    dir = vec3(1, 0, 0);
    up = vec3(0, 0, -1);
    
    // apply rotation
    {
        double yaw = mapConfig->srs.get(
                    mapConfig->referenceFrame.model.navigationSrs).type
                == vtslibs::registry::Srs::Type::projected
                ? rot(0) : -rot(0);
        mat3 tmp = upperLeftSubMatrix(rotationMatrix(2, yaw))
                * upperLeftSubMatrix(rotationMatrix(1, -rot(1)))
                * upperLeftSubMatrix(rotationMatrix(0, -rot(2)));
        dir = tmp * dir;
        up = tmp * up;
    }
    
    // transform to physical srs
    switch (mapConfig->srs.get
            (mapConfig->referenceFrame.model.navigationSrs).type)
    {
    case vtslibs::registry::Srs::Type::projected:
    {
        // swap XY
        std::swap(dir(0), dir(1));
        std::swap(up(0), up(1));
        // invert Z
        dir(2) *= -1;
        up(2) *= -1;
        // add center of orbit (transform to navigation srs)
        dir += center;
        up += center;
        // transform to physical srs
        center = convertor->navToPhys(center);
        dir = convertor->navToPhys(dir);
        up = convertor->navToPhys(up);
        // points -> vectors
        dir = normalize(dir - center);
        up = normalize(up - center);
    } break;
    case vtslibs::registry::Srs::Type::geographic:
    {
        // find lat-lon coordinates of points moved to north and east
        vec3 n2 = convertor->geoDirect(center, 100, 0);
        vec3 e2 = convertor->geoDirect(center, 100, 90);
        // transform to physical srs
        center = convertor->navToPhys(center);
        vec3 n = convertor->navToPhys(n2);
        vec3 e = convertor->navToPhys(e2);
        // points -> vectors
        n = normalize(n - center);
        e = normalize(e - center);
        // construct NED coordinate system
        vec3 d = normalize(cross(n, e));
        e = normalize(cross(n, d));
        mat3 tmp = (mat3() << n, e, d).finished();
        // rotate original vectors
        dir = tmp * dir;
        up = tmp * up;
        dir = normalize(dir);
        up = normalize(up);
    } break;
    default:
        LOGTHROW(fatal, std::invalid_argument) << "Invalid srs type";
    }
}

double MapImpl::positionObjectiveDistance()
{
    vtslibs::registry::Position &pos = mapConfig->position;
    return pos.verticalExtent
            * 0.5 / tan(degToRad(pos.verticalFov * 0.5));
}

void MapImpl::initializeNavigation()
{
    convertor = CoordManip::create(
                  mapConfig->referenceFrame.model.physicalSrs,
                  mapConfig->referenceFrame.model.navigationSrs,
                  mapConfig->referenceFrame.model.publicSrs,
                  *mapConfig);

    navigation.targetPoint = vecFromUblas<vec3>(mapConfig->position.position);
    navigation.changeRotation = vec3(0,0,0);
    navigation.targetViewExtent = mapConfig->position.verticalExtent;
    navigation.autoRotation(0) = mapConfig->browserOptions.autorotate;
    for (int i = 0; i < 3; i++)
        normalizeAngle(mapConfig->position.orientation[i]);
}

void MapImpl::updateNavigation()
{
    assert(options.cameraInertiaPan >= 0
           && options.cameraInertiaPan < 1);
    assert(options.cameraInertiaRotate >= 0
           && options.cameraInertiaRotate < 1);
    assert(options.cameraInertiaZoom >= 0
           && options.cameraInertiaZoom < 1);
    assert(options.navigationLatitudeThreshold > 0
           && options.navigationLatitudeThreshold < 90);

    checkPanZQueue();

    vtslibs::registry::Position &pos = mapConfig->position;
    vec3 p = vecFromUblas<vec3>(pos.position);
    vec3 r = vecFromUblas<vec3>(pos.orientation);

    // floating position
    if (pos.heightMode == vtslibs::registry::Position::HeightMode::floating)
    {
        pos.heightMode = vtslibs::registry::Position::HeightMode::fixed;
        resetPositionAltitude(p(2));
    }
    assert(pos.heightMode == vtslibs::registry::Position::HeightMode::fixed);

    // limit zoom
    navigation.targetViewExtent = clamp(navigation.targetViewExtent,
                                       options.positionViewExtentMin,
                                       options.positionViewExtentMax);

    // auto
    navigation.changeRotation += navigation.autoRotation;
    panImpl(navigation.autoMotion, 1);
    
    // apply pan
    switch (mapConfig->navigationType())
    {
    case vtslibs::registry::Srs::Type::projected:
    {
        p += (navigation.targetPoint - p)
                * (1 - options.cameraInertiaPan);
    } break;
    case vtslibs::registry::Srs::Type::geographic:
    {
        // try to switch to free mode
        if (options.navigationMode == MapOptions::NavigationMode::Dynamic)
        {
            if (abs(navigation.targetPoint(1))
                    > options.navigationLatitudeThreshold - 1e-5)
                navigation.mode = MapOptions::NavigationMode::Free;
        }
        else
            navigation.mode = options.navigationMode;

        // move
        switch (navigation.mode)
        {
        case MapOptions::NavigationMode::Free:
        {
            double dist, azi1, azi2;
            convertor->geoInverse(p, navigation.targetPoint, dist, azi1, azi2);
            dist *= (1 - options.cameraInertiaPan);
            p = convertor->geoDirect(p, dist, azi1, azi2);
            r(0) += azi2 - azi1;
        } break;
        case MapOptions::NavigationMode::Azimuthal:
        {
            navigation.targetPoint(1) = clamp(navigation.targetPoint(1),
                    -options.navigationLatitudeThreshold,
                    options.navigationLatitudeThreshold);
            for (int i = 0; i < 2; i++)
                p(i) += angularDiff(p(i) + 180, navigation.targetPoint(i) + 180)
                    * (1 - options.cameraInertiaPan);
        } break;
        default:
            LOGTHROW(fatal, std::invalid_argument) << "invalid navigation mode";
        }
        
        p(0) = modulo(p(0) + 180, 360) - 180;
        p(2) += (navigation.targetPoint(2) - p(2))
                * (1 - options.cameraInertiaPan);
    } break;
    default:
        LOGTHROW(fatal, std::invalid_argument) << "invalid navigation srs";
    }

    // apply rotation & zoom inertia
    r += navigation.changeRotation
            * (1 - options.cameraInertiaRotate);
    navigation.changeRotation *= options.cameraInertiaRotate;
    pos.verticalExtent += (navigation.targetViewExtent - pos.verticalExtent)
            * (1 - options.cameraInertiaZoom);
    
    for (int i = 0; i < 3; i++)
        normalizeAngle(r[i]);
    r[1] = clamp(r[1], 270, 350);

    // asserts
    assert(r(0) >= 0 && r(0) < 360);
    assert(r(1) >= 0 && r(1) < 360);
    assert(r(2) >= 0 && r(2) < 360);
    if (mapConfig->navigationType() == vtslibs::registry::Srs::Type::geographic)
    {
        assert(p(0) >= -180 && p(0) <= 180);
        assert(p(1) >= -90 && p(1) <= 90);
    }

    // vertical camera adjustment
    {
        auto h = std::make_shared<HeightRequest>(vec3to2(p));
        if (navigation.panZQueue.size() < 2)
            navigation.panZQueue.push(h);
        else
            navigation.panZQueue.back() = h;
    }

    // store changed values
    pos.position = vecToUblas<math::Point3>(p);
    pos.orientation = vecToUblas<math::Point3>(r);
}

void MapImpl::pan(const vec3 &value)
{
    panImpl(value, options.cameraSensitivityPan);
    resetAuto();
}

void MapImpl::panImpl(const vec3 &value, double speed)
{
    vtslibs::registry::Position &pos = mapConfig->position;

    double h = 1;
    if (mapConfig->navigationType() == vtslibs::registry::Srs::Type::geographic
            && navigation.mode == MapOptions::NavigationMode::Azimuthal)
    {
        // slower pan near poles
        h = std::cos(pos.position[1] * 3.14159 / 180);
    }
    
    // pan speed depends on zoom level
    double v = pos.verticalExtent / 800;
    vec3 move = value.cwiseProduct(vec3(-2 * v * h, 2 * v, 2) * speed);

    double azi = pos.orientation(0);
    if (mapConfig->navigationType() == vtslibs::registry::Srs::Type::geographic
            && navigation.mode == MapOptions::NavigationMode::Free)
    {
        // camera rotation taken from current (aka previous) target position
        // this prevents strange turning near poles
        double d, a1, a2;
        convertor->geoInverse(
                    vecFromUblas<vec3>(pos.position),
                    navigation.targetPoint, d, a1, a2);
        azi += a2 - a1;
    }
    
    // the move is rotated by the camera
    mat3 rot = upperLeftSubMatrix(rotationMatrix(2, -azi));
    move = rot * move;

    switch (mapConfig->navigationType())
    {
    case vtslibs::registry::Srs::Type::projected:
    {
        navigation.targetPoint += move;
        // todo periodicity
    } break;
    case vtslibs::registry::Srs::Type::geographic:
    {
        vec3 p = navigation.targetPoint;
        double ang1 = radToDeg(atan2(move(0), move(1)));
        double ang2;
        double dist = length(vec3to2(move));
        p = convertor->geoDirect(p, dist, ang1, ang2);
        p(0) = modulo(p(0) + 180, 360) - 180;
        p(2) += move(2);
        // ignore the pan, if it would cause too rapid direction change
        switch (navigation.mode)
        {
        case MapOptions::NavigationMode::Azimuthal:
            if (abs(angularDiff(pos.position[0] + 180, p(0) + 180)) < 90)
                navigation.targetPoint = p;
            break;
        case MapOptions::NavigationMode::Free:
            if (convertor->geoArcDist(vecFromUblas<vec3>(pos.position), p) < 90)
                navigation.targetPoint = p;
            break;
        default:
            LOGTHROW(fatal, std::runtime_error) << "invalid navigation mode";
        }
    } break;
    default:
        LOGTHROW(fatal, std::runtime_error) << "invalid navigation srs";
    }
}

void MapImpl::rotate(const vec3 &value)
{
    navigation.changeRotation += value.cwiseProduct(vec3(0.2, -0.1, 0.2)
                                        * options.cameraSensitivityRotate);
    if (options.navigationMode == MapOptions::NavigationMode::Dynamic)
        navigation.mode = MapOptions::NavigationMode::Free;
    resetAuto();
}

void MapImpl::zoom(double value)
{
    double c = value * options.cameraSensitivityZoom;
    navigation.targetViewExtent *= pow(1.001, -c);
    resetAuto();
}

void MapImpl::setPoint(const vec3 &point, bool immediate)
{
    navigation.targetPoint = point;
    if (immediate)
    {
        mapConfig->position.position = vecToUblas<math::Point3>(point);
        navigation.lastPanZShift.reset();
        std::queue<std::shared_ptr<class HeightRequest>>()
                .swap(navigation.panZQueue) ;
    }
    resetAuto();
}

void MapImpl::setRotation(const vec3 &euler, bool immediate)
{
    if (immediate)
    {
        mapConfig->position.orientation = vecToUblas<math::Point3>(euler);
        navigation.changeRotation = vec3(0,0,0);
    }
    else
    {
        navigation.changeRotation = angularDiff(
                    vecFromUblas<vec3>(mapConfig->position.orientation), euler);
    }
    resetAuto();
}

void MapImpl::setViewExtent(double viewExtent, bool immediate)
{
    navigation.targetViewExtent = viewExtent;
    if (immediate)
        mapConfig->position.verticalExtent = viewExtent;
    resetAuto();
}

} // namespace vts
