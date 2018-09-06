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
#include "navigationPiha.hpp"

namespace vts
{

MapImpl::Navigation::Navigation() :
    changeRotation(0,0,0), targetPoint(0,0,0), autoRotation(0),
    targetViewExtent(0), mode(NavigationMode::Azimuthal),
    previousType(NavigationType::Quick)
{}

uint32 MapImpl::applyCameraRotationNormalization(vec3 &rot)
{
    if (!options.cameraNormalization
            || options.navigationType == NavigationType::FlyOver
            || mapConfig->position.type
                    == vtslibs::registry::Position::Type::subjective)
        return 0;

    // find the interpolation factor
    double extCur = mapConfig->position.verticalExtent;
    double extLow = options.viewExtentThresholdScaleLow * body.majorRadius;
    double extHig = options.viewExtentThresholdScaleHigh * body.majorRadius;
    extCur = std::log2(extCur);
    extLow = std::log2(extLow);
    extHig = std::log2(extHig);
    double f = (extCur - extLow) / (extHig - extLow);
    f = clamp(f, 0, 1);
    f = smootherstep(f);

    // tilt limit
    rot(1) = interpolate(rot(1), options.tiltLimitAngleLow, f);

    // yaw limit
    double &yaw = rot(0);
    if (options.navigationMode == NavigationMode::Azimuthal)
        yaw = 0;
    else if (options.navigationMode == NavigationMode::Seamless)
        yaw = interpolate(yaw, 0, f);

    return /*(options.navigationMode == NavigationMode::Seamless
                || options.navigationMode == NavigationMode::Azimuthal)
            &&*/ extCur > extHig ? 2 : 1;
}

uint32 MapImpl::applyCameraRotationNormalization(math::Point3 &rot)
{
    vec3 r = vecFromUblas<vec3>(rot);
    auto res = applyCameraRotationNormalization(r);
    rot = vecToUblas<math::Point3>(r);
    return res;
}

void MapImpl::resetNavigationMode()
{
    if (options.navigationMode == NavigationMode::Azimuthal
            || options.navigationMode == NavigationMode::Free)
        navigation.mode = options.navigationMode;
    else
        navigation.mode = NavigationMode::Azimuthal;
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
    applyCameraRotationNormalization(rot);
    center = vecFromUblas<vec3>(pos.position);
    dir = vec3(1, 0, 0);
    up = vec3(0, 0, -1);

    // apply rotation
    {
        double yaw = mapConfig->srs.get(
                    mapConfig->referenceFrame.model.navigationSrs).type
                == vtslibs::registry::Srs::Type::projected
                ? rot(0) : -rot(0);
        mat3 tmp = mat4to3(rotationMatrix(2, yaw))
                 * mat4to3(rotationMatrix(1, -rot(1)))
                 * mat4to3(rotationMatrix(0, -rot(2)));
        dir = tmp * dir;
        up = tmp * up;
    }

    // transform to physical srs
    switch (mapConfig->navigationSrsType())
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
        LOGTHROW(fatal, std::invalid_argument)
                << "Invalid navigation srs type";
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
    navigation.targetPoint = vecFromUblas<vec3>(mapConfig->position.position);
    navigation.changeRotation = vec3(0,0,0);
    navigation.targetViewExtent = mapConfig->position.verticalExtent;
    navigation.autoRotation = mapConfig->browserOptions.autorotate;
    for (int i = 0; i < 3; i++)
        normalizeAngle(mapConfig->position.orientation[i]);

    assert(isNavigationModeValid());
}

void MapImpl::navigationTypeChanged()
{
    if (navigation.previousType != options.navigationType)
    {
        // fly over must first apply the old camera limits
        std::swap(navigation.previousType, options.navigationType);
        applyCameraRotationNormalization(mapConfig->position.orientation);

        // store the new value
        options.navigationType = navigation.previousType;
    }
}

void MapImpl::updateNavigation(double elapsedTime)
{
    {
        const ControlOptions &co = options.controlOptions;
        (void)co;
        assert(co.inertiaPan >= 0
               && co.inertiaPan < 1);
        assert(co.inertiaRotate >= 0
               && co.inertiaRotate < 1);
        assert(co.inertiaZoom >= 0
               && co.inertiaZoom < 1);
    }
    assert(options.navigationLatitudeThreshold > 0
           && options.navigationLatitudeThreshold < 90);

    navigation.renders.clear();

    vtslibs::registry::Position &pos = mapConfig->position;

    // convert floating position
    if (pos.heightMode != vtslibs::registry::Position::HeightMode::fixed)
    {
        pos.heightMode = vtslibs::registry::Position::HeightMode::fixed;
        navigation.positionAltitudeReset.emplace(pos.position[2]);
        pos.position[2] = 0;
    }

    // check if navigation type has changed
    navigationTypeChanged();

    vec3 p = vecFromUblas<vec3>(pos.position);
    vec3 r = vecFromUblas<vec3>(pos.orientation);

    // limit zoom
    navigation.targetViewExtent = clamp(navigation.targetViewExtent,
           options.viewExtentLimitScaleMin * body.majorRadius,
           options.viewExtentLimitScaleMax * body.majorRadius);

    // update navigation mode
    switch (options.navigationMode)
    {
    case NavigationMode::Azimuthal:
    case NavigationMode::Free:
        navigation.mode = options.navigationMode;
        break;
    case NavigationMode::Dynamic:
        if (mapConfig->navigationSrsType()
                == vtslibs::registry::Srs::Type::projected)
            navigation.mode = NavigationMode::Azimuthal;
        else
        {
            // switch to free mode when too close to a pole
            if (std::abs(navigation.targetPoint(1))
                    > options.navigationLatitudeThreshold - 1e-5)
                navigation.mode = NavigationMode::Free;
        }
        break;
    case NavigationMode::Seamless:
        navigation.mode = mapConfig->position.verticalExtent
                < options.viewExtentThresholdScaleLow * body.majorRadius
                ? NavigationMode::Free : NavigationMode::Azimuthal;
        break;
    default:
        LOGTHROW(fatal, std::invalid_argument)
                << "Invalid navigation mode";
    }

    // limit latitude in azimuthal navigation
    if (navigation.mode == NavigationMode::Azimuthal
            && mapConfig->navigationSrsType()
                    != vtslibs::registry::Srs::Type::projected)
    {
        navigation.targetPoint(1) = clamp(navigation.targetPoint(1),
                -options.navigationLatitudeThreshold,
                options.navigationLatitudeThreshold);
    }
    assert(isNavigationModeValid());

    // auto rotation
    navigation.changeRotation(0) += navigation.autoRotation;

    // limit rotation for seamless navigation mode
    if (options.cameraNormalization
            && options.navigationMode == NavigationMode::Seamless
            && navigation.mode == NavigationMode::Azimuthal
            && mapConfig->position.type
                    == vtslibs::registry::Position::Type::objective)
    {
        double yaw = mapConfig->position.orientation[0];
        double &ch = navigation.changeRotation(0);
        ch = clamp(yaw + ch, -180 + 1e-7, 180 - 1e-7) - yaw;
        assert(yaw + ch >= -180 && yaw + ch <= 180);
    }

    // find inputs for perceptually invariant motion
    double azi1 = std::numeric_limits<double>::quiet_NaN();
    double azi2 = std::numeric_limits<double>::quiet_NaN();
    double horizontal1 = std::numeric_limits<double>::quiet_NaN();
    double horizontal2 = std::numeric_limits<double>::quiet_NaN();
    switch (mapConfig->navigationSrsType())
    {
    case vtslibs::registry::Srs::Type::projected:
        horizontal1 = length(vec2(
                        vec3to2(navigation.targetPoint) - vec3to2(p)));
        break;
    case vtslibs::registry::Srs::Type::geographic:
        convertor->geoInverse(p, navigation.targetPoint, horizontal1,
                              azi1, azi2);
        break;
    default:
        LOGTHROW(fatal, std::invalid_argument)
                << "Invalid navigation srs type";
    }
    double vertical1 = navigation.targetPoint(2) - p(2);
    double vertical2 = std::numeric_limits<double>::quiet_NaN();
    vec3 r2(vertical2, vertical2, vertical2);
    double prevExtent = pos.verticalExtent;
    navigationPiha(
                options,
                elapsedTime,
                pos.verticalFov,
                horizontal1,
                vertical1,
                prevExtent,
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
    {
        switch (mapConfig->navigationSrsType())
        {
        case vtslibs::registry::Srs::Type::projected:
        {
            p += (navigation.targetPoint - p) * (horizontal2 / horizontal1);
        } break;
        case vtslibs::registry::Srs::Type::geographic:
        {
            switch (navigation.mode)
            {
            case NavigationMode::Free:
            {
                p = convertor->geoDirect(p, horizontal2, azi1, azi2);
                r(0) += azi2 - azi1;
            } break;
            case NavigationMode::Azimuthal:
            {
                for (int i = 0; i < 2; i++)
                    p(i) += angularDiff(p(i), navigation.targetPoint(i))
                         * (horizontal2 / horizontal1);
            } break;
            default:
                LOGTHROW(fatal, std::invalid_argument)
                        << "Invalid navigation mode";
            }
        } break;
        default:
            LOGTHROW(fatal, std::invalid_argument)
                    << "Invalid navigation srs type";
        }
    }

    // apply periodicity
    {
        vec3 pp = p;
        switch (mapConfig->navigationSrsType())
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
        default:
            LOGTHROW(fatal, std::invalid_argument)
                    << "Invalid navigation srs type";
        }
        navigation.targetPoint += p - pp;
    }

    // normalize rotation
    if (options.cameraNormalization
            && pos.type == vtslibs::registry::Position::Type::objective
            && mapConfig->position.type
                    == vtslibs::registry::Position::Type::objective)
    {
        r[1] = clamp(r[1],
            options.tiltLimitAngleLow,
            options.tiltLimitAngleHigh);
    }
    for (int i = 0; i < 3; i++)
        normalizeAngle(r[i]);

    // asserts
    assert(isNavigationModeValid());
    assert(r(0) >= -180 && r(0) <= 180);
    assert(r(1) >= -180 && r(1) <= 180);
    assert(r(2) >= -180 && r(2) <= 180);
    if (mapConfig->navigationSrsType()
            == vtslibs::registry::Srs::Type::geographic)
    {
        assert(p(0) >= -180 && p(0) <= 180);
        assert(p(1) >= -90 && p(1) <= 90);
    }

    // store changed values
    pos.position = vecToUblas<math::Point3>(p);
    pos.orientation = vecToUblas<math::Point3>(r);

    // altitude corrections
    if (pos.type == vtslibs::registry::Position::Type::objective)
    {
        double f1 = horizontal2 / pos.verticalExtent;
        double f2  = std::abs(std::log(pos.verticalExtent)
                              - std::log(prevExtent));
        updatePositionAltitude(std::max(f1, f2));
    }

    // statistics
    statistics.currentNavigationMode = navigation.mode;
}

bool MapImpl::isNavigationModeValid() const
{
    if (navigation.mode != NavigationMode::Azimuthal
            && navigation.mode != NavigationMode::Free)
        return false;
    return true;
}

vec3 MapImpl::applyCameraRotationNormalizationPermanently()
{
    vtslibs::registry::Position &pos = mapConfig->position;
    vec3 posRot = vecFromUblas<vec3>(pos.orientation);
    if (applyCameraRotationNormalization(posRot) == 2)
    {
        pos.orientation = vecToUblas<math::Point3>(posRot);
        if (navigation.mode == NavigationMode::Azimuthal)
            navigation.changeRotation = vec3(0,0,0);
    }
    return posRot;
}

void MapImpl::pan(vec3 value)
{
    assert(isNavigationModeValid());

    vtslibs::registry::Position &pos = mapConfig->position;
    vec3 posRot = applyCameraRotationNormalizationPermanently();

    // camera roll
    value = mat4to3(rotationMatrix(2, -posRot[2])) * value;

    // slower movement near poles
    double h = 1;
    if (mapConfig->navigationSrsType()
            == vtslibs::registry::Srs::Type::geographic
            && navigation.mode == NavigationMode::Azimuthal)
    {
        h = std::cos(degToRad(pos.position[1]));
    }

    // pan speed depends on zoom level
    double v = pos.verticalExtent / 600;
    vec3 move = value.cwiseProduct(vec3(-2 * v * h, 2 * v, 2)
                    * options.controlOptions.sensitivityPan);

    // compute change of azimuth
    double azi = posRot[0];
    if (mapConfig->navigationSrsType()
                == vtslibs::registry::Srs::Type::geographic)
    {
        // camera rotation taken from previous target position
        // this prevents strange turning near poles
        double d, a1, a2;
        convertor->geoInverse(
                    vecFromUblas<vec3>(pos.position),
                    navigation.targetPoint, d, a1, a2);
        azi += a2 - a1;
    }
    move = mat4to3(rotationMatrix(2, -azi)) * move;

    // apply the pan
    switch (mapConfig->navigationSrsType())
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

        // prevent the pan if it would cause unexpected direction change
        double r1 = angularDiff(pos.position[0], p(0));
        double r2 = angularDiff(pos.position[0], navigation.targetPoint[0]);
        if (std::abs(r1) < 30 || (r1 < 0) == (r2 < 0))
            navigation.targetPoint = p;
    } break;
    default:
        LOGTHROW(fatal, std::invalid_argument)
                << "Invalid navigation srs type";
    }

    navigation.autoRotation = 0;

    assert(isNavigationModeValid());
}

void MapImpl::rotate(vec3 value)
{
    assert(isNavigationModeValid());

    if (mapConfig->navigationSrsType()
            == vtslibs::registry::Srs::Type::geographic
            && options.navigationMode == NavigationMode::Dynamic)
        navigation.mode = NavigationMode::Free;

    navigation.changeRotation += value.cwiseProduct(vec3(0.2, -0.1, 0.2)
                    * options.controlOptions.sensitivityRotate);
    applyCameraRotationNormalizationPermanently();
    navigation.autoRotation = 0;

    assert(isNavigationModeValid());
}

void MapImpl::zoom(double value)
{
    assert(isNavigationModeValid());

    double c = value * options.controlOptions.sensitivityZoom * 120;
    navigation.targetViewExtent *= std::pow(1.002, -c);
    navigation.autoRotation = 0;

    assert(isNavigationModeValid());
}

void MapImpl::setPoint(const vec3 &point)
{
    assert(isNavigationModeValid());

    navigation.targetPoint = point;
    navigation.autoRotation = 0;
    if (options.navigationType == NavigationType::Instant)
        navigation.lastPositionAltitude.reset();
}

void MapImpl::setRotation(const vec3 &euler)
{
    assert(isNavigationModeValid());

    navigation.changeRotation = angularDiff(
                vecFromUblas<vec3>(mapConfig->position.orientation), euler);
    navigation.autoRotation = 0;
}

void MapImpl::setViewExtent(double viewExtent)
{
    assert(isNavigationModeValid());

    navigation.targetViewExtent = viewExtent;
    navigation.autoRotation = 0;
}

} // namespace vts
