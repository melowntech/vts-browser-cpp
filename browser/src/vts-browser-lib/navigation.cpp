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
#include "piha.hpp"

namespace vts
{

MapImpl::Navigation::Navigation() :
    changeRotation(0,0,0), targetPoint(0,0,0), autoRotation(0),
    targetViewExtent(0), geographicMode(NavigationGeographicMode::Azimuthal),
    type(NavigationType::Quick)
{}

void MapImpl::resetNavigationGeographicMode()
{
    if (options.geographicNavMode == NavigationGeographicMode::Dynamic)
        navigation.geographicMode = NavigationGeographicMode::Azimuthal;
    else
        navigation.geographicMode = options.geographicNavMode;
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
    navigation.targetPoint = vecFromUblas<vec3>(pos.position);
}

void MapImpl::positionToCamera(vec3 &center, vec3 &dir, vec3 &up)
{
    vtslibs::registry::Position &pos = mapConfig->position;
    
    // camera-space vectors
    vec3 rot = vecFromUblas<vec3>(pos.orientation);
    applyPositionTiltLimit(rot(1));
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
    switch (mapConfig->navigationType())
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
    case vtslibs::registry::Srs::Type::cartesian:
        LOGTHROW(fatal, std::invalid_argument) << "Invalid navigation srs type";
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
                mapConfig->browserOptions.searchSrs,
                *mapConfig);

    navigation.targetPoint = vecFromUblas<vec3>(mapConfig->position.position);
    navigation.changeRotation = vec3(0,0,0);
    navigation.targetViewExtent = mapConfig->position.verticalExtent;
    navigation.autoRotation = mapConfig->browserOptions.autorotate;
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

    navigation.draws.clear();

    updatePositionAltitudeShift();

    vtslibs::registry::Position &pos = mapConfig->position;
    vec3 p = vecFromUblas<vec3>(pos.position);
    vec3 r = vecFromUblas<vec3>(pos.orientation);

    // floating position
    if (pos.heightMode == vtslibs::registry::Position::HeightMode::floating)
        pos.heightMode = vtslibs::registry::Position::HeightMode::fixed;
    assert(pos.heightMode == vtslibs::registry::Position::HeightMode::fixed);

    // limit zoom
    navigation.targetViewExtent = clamp(navigation.targetViewExtent,
                                       options.positionViewExtentMin,
                                       options.positionViewExtentMax);

    if (mapConfig->navigationType() == vtslibs::registry::Srs::Type::geographic)
    {
        // check navigation mode
        if (options.geographicNavMode == NavigationGeographicMode::Dynamic)
        {
            // too close to pole -> switch to free mode
            if (abs(navigation.targetPoint(1))
                    > options.navigationLatitudeThreshold - 1e-5)
                navigation.geographicMode = NavigationGeographicMode::Free;
        }
        else
            navigation.geographicMode = options.geographicNavMode;
    
        // limit latitude in azimuthal navigation
        if (navigation.geographicMode == NavigationGeographicMode::Azimuthal)
        {
            navigation.targetPoint(1) = clamp(navigation.targetPoint(1),
                    -options.navigationLatitudeThreshold,
                    options.navigationLatitudeThreshold);
        }
    }

    // auto rotation
    navigation.changeRotation(0) += navigation.autoRotation;
    
    // find inputs for perceptually invariant motion
    double azi1 = std::numeric_limits<double>::quiet_NaN();
    double azi2 = std::numeric_limits<double>::quiet_NaN();
    double horizontal1 = std::numeric_limits<double>::quiet_NaN();
    double horizontal2 = std::numeric_limits<double>::quiet_NaN();
    switch (mapConfig->navigationType())
    {
    case vtslibs::registry::Srs::Type::projected:
        horizontal1 = length(vec2(
                        vec3to2(navigation.targetPoint) - vec3to2(p)));
        break;
    case vtslibs::registry::Srs::Type::geographic:
        convertor->geoInverse(p, navigation.targetPoint, horizontal1,
                              azi1, azi2);
        break;
    case vtslibs::registry::Srs::Type::cartesian:
        LOGTHROW(fatal, std::invalid_argument) << "Invalid navigation srs type";
    }
    double vertical1 = navigation.targetPoint(2) - p(2);
    double vertical2 = std::numeric_limits<double>::quiet_NaN();
    vec3 r2;
    navigationPiha(
                options,
                navigation.type,
                1.0 / 60.0, // todo
                pos.verticalFov,
                horizontal1,
                vertical1,
                pos.verticalExtent,
                navigation.targetViewExtent - pos.verticalExtent,
                r,
                navigation.changeRotation,
                pos.verticalExtent,
                horizontal2,
                vertical2,
                r2
                );

    // vertical move
    p(2) += vertical2;
    
    // rotation
    navigation.changeRotation -= r2 - r;
    r = r2;
    
    // horizontal move
    if (horizontal1 > 0)
    switch (mapConfig->navigationType())
    {
    case vtslibs::registry::Srs::Type::projected:
    {
        p += (navigation.targetPoint - p) * (horizontal2 / horizontal1);
    } break;
    case vtslibs::registry::Srs::Type::geographic:
    {
        switch (navigation.geographicMode)
        {
        case NavigationGeographicMode::Free:
        {
            p = convertor->geoDirect(p, horizontal2, azi1, azi2);
            r(0) += azi2 - azi1;
        } break;
        case NavigationGeographicMode::Azimuthal:
        {
            for (int i = 0; i < 2; i++)
                p(i) += angularDiff(p(i), navigation.targetPoint(i))
                     * (horizontal2 / horizontal1);
        } break;
        case NavigationGeographicMode::Dynamic:
            LOGTHROW(fatal, std::invalid_argument) << "Invalid navigation mode";
        }
    } break;
    case vtslibs::registry::Srs::Type::cartesian:
        LOGTHROW(fatal, std::invalid_argument) << "Invalid navigation srs type";
    }

    // apply periodicity
    {
        vec3 pp = p;
        switch (mapConfig->navigationType())
        {
        case vtslibs::registry::Srs::Type::projected:
        {
            const vtslibs::registry::Srs &srs = mapConfig->srs.get(
                        mapConfig->referenceFrame.model.navigationSrs);
            if (srs.periodicity)
            {
                const vtslibs::registry::Periodicity &pr = *srs.periodicity;
                int axis = -1;
                switch (pr.type)
                {
                case vtslibs::registry::Periodicity::Type::x:
                    axis = 0;
                    break;
                case vtslibs::registry::Periodicity::Type::y:
                    axis = 1;
                    break;
                }
                p(axis) = modulo(p(axis) + pr.period * 0.5, pr.period)
                        - pr.period * 0.5;
            }
        } break;
        case vtslibs::registry::Srs::Type::geographic:
        {
            p(0) = modulo(p(0) + 180, 360) - 180;
        } break;
        case vtslibs::registry::Srs::Type::cartesian:
            LOGTHROW(fatal, std::invalid_argument)
                    << "Invalid navigation srs type";
        }
        navigation.targetPoint += p - pp;
    }
    
    // normalize rotation
    for (int i = 0; i < 3; i++)
        normalizeAngle(r[i]);
    if (navigation.type == NavigationType::FlyOver)
        applyPositionTiltLimit(r[1]);
    r[1] = clamp(r[1], options.positionTiltLimitLow,
            options.positionTiltLimitHigh);

    // asserts
    assert(r(0) >= 0 && r(0) < 360);
    assert(r(1) >= 0 && r(1) < 360);
    assert(r(2) >= 0 && r(2) < 360);
    if (mapConfig->navigationType() == vtslibs::registry::Srs::Type::geographic)
    {
        assert(p(0) >= -180 && p(0) <= 180);
        assert(p(1) >= -90 && p(1) <= 90);
    }

    // store changed values
    pos.position = vecToUblas<math::Point3>(p);
    pos.orientation = vecToUblas<math::Point3>(r);
}

void MapImpl::pan(const vec3 &value)
{
    vtslibs::registry::Position &pos = mapConfig->position;

    double h = 1;
    if (mapConfig->navigationType() == vtslibs::registry::Srs::Type::geographic
            && navigation.geographicMode == NavigationGeographicMode::Azimuthal)
    {
        // slower pan near poles
        h = std::cos(pos.position[1] * 3.14159 / 180);
    }

    // pan speed depends on zoom level
    double v = pos.verticalExtent / 800;
    vec3 move = value.cwiseProduct(vec3(-2 * v * h, 2 * v, 2)
                                   * options.cameraSensitivityPan);

    double azi = pos.orientation(0);
    if (mapConfig->navigationType() == vtslibs::registry::Srs::Type::geographic
            && navigation.geographicMode == NavigationGeographicMode::Free)
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
    } break;
    case vtslibs::registry::Srs::Type::geographic:
    {
        vec3 p = navigation.targetPoint;
        double ang1 = radToDeg(atan2(move(0), move(1)));
        double dist = length(vec3to2(move));
        p = convertor->geoDirect(p, dist, ang1);
        p(2) += move(2);
        // ignore the pan, if it would cause too rapid direction change
        switch (navigation.geographicMode)
        {
        case NavigationGeographicMode::Azimuthal:
            if (abs(angularDiff(pos.position[0], p(0))) < 150)
                navigation.targetPoint = p;
            break;
        case NavigationGeographicMode::Free:
            if (convertor->geoArcDist(vecFromUblas<vec3>(pos.position), p) <150)
                navigation.targetPoint = p;
            break;
        case NavigationGeographicMode::Dynamic:
            LOGTHROW(fatal, std::invalid_argument) << "Invalid navigation mode";
        }
    } break;
    case vtslibs::registry::Srs::Type::cartesian:
        LOGTHROW(fatal, std::invalid_argument) << "Invalid navigation srs type";
    }

    navigation.autoRotation = 0;
    navigation.type = options.navigationType;
}

void MapImpl::rotate(const vec3 &value)
{
    applyPositionTiltLimit(mapConfig->position.orientation[1]);
    navigation.changeRotation += value.cwiseProduct(vec3(0.2, -0.1, 0.2)
                                        * options.cameraSensitivityRotate);
    if (options.geographicNavMode == NavigationGeographicMode::Dynamic)
        navigation.geographicMode = NavigationGeographicMode::Free;
    navigation.autoRotation = 0;
    navigation.type = options.navigationType;
}

void MapImpl::zoom(double value)
{
    double c = value * options.cameraSensitivityZoom;
    navigation.targetViewExtent *= pow(1.001, -c);
    navigation.autoRotation = 0;
    navigation.type = options.navigationType;
}

void MapImpl::setPoint(const vec3 &point, NavigationType type)
{
    navigation.targetPoint = point;
    navigation.autoRotation = 0;
    navigation.type = type;
    if (navigation.type == NavigationType::Instant)
        navigation.lastPositionAltitudeShift.reset();
}

void MapImpl::setRotation(const vec3 &euler, NavigationType type)
{
    navigation.changeRotation = angularDiff(
                    vecFromUblas<vec3>(mapConfig->position.orientation), euler);
    navigation.autoRotation = 0;
    navigation.type = type;
}

void MapImpl::setViewExtent(double viewExtent, NavigationType type)
{
    navigation.targetViewExtent = viewExtent;
    navigation.autoRotation = 0;
    navigation.type = type;
}

} // namespace vts
