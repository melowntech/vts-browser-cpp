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

#include <optick.h>

#include "../include/vts-browser/camera.hpp"

#include "../navigation.hpp"
#include "../camera.hpp"
#include "../mapConfig.hpp"
#include "../map.hpp"
#include "../coordsManip.hpp"
#include "../renderTasks.hpp"
#include "../gpuResource.hpp"
#include "solver.hpp"

namespace vts
{

NavigationImpl::NavigationImpl(CameraImpl *cam, Navigation *navigation) :
    camera(cam),
    navigation(navigation),
    position(0, 0, 0),
    targetPosition(0, 0, 0),
    orientation(0, 0, 0),
    targetOrientation(0, 0, 0),
    verticalExtent(0),
    targetVerticalExtent(0),
    verticalFov(0),
    autoRotation(0),
    type(Type::objective),
    heightMode(HeightMode::fixed),
    mode(NavigationMode::Azimuthal),
    suspendAltitudeChange(false)
{
    if (camera->map->mapconfig && *camera->map->mapconfig)
        initialize();
}

void NavigationImpl::initialize()
{
    assert(*camera->map->mapconfig);

    setPosition(camera->map->mapconfig->position);
    position = targetPosition;
    orientation = targetOrientation;
    verticalExtent = targetVerticalExtent;
    autoRotation = camera->map->mapconfig->browserOptions.autorotate;

    assert(isNavigationModeValid());
}

void NavigationImpl::pan(vec3 value)
{
    assert(isNavigationModeValid());

    // camera roll
    value = mat4to3(rotationMatrix(2, -orientation[2])) * value;

    // slower movement near poles
    double h = 1;
    if (camera->map->mapconfig->navigationSrsType()
        == vtslibs::registry::Srs::Type::geographic
        && mode == NavigationMode::Azimuthal)
    {
        h = std::cos(degToRad(position[1]));
    }

    // pan speed depends on zoom level
    double v = verticalExtent / 600;
    vec3 move = value.cwiseProduct(vec3(-2 * v * h, 2 * v, 2)
                    * options.sensitivityPan);

    // compute change of azimuth
    double azi = orientation[0];
    if (camera->map->mapconfig->navigationSrsType()
        == vtslibs::registry::Srs::Type::geographic)
    {
        // camera rotation taken from previous target position
        // this prevents strange turning near poles
        double d, a1, a2;
        camera->map->convertor->geoInverse(position,
                    targetPosition, d, a1, a2);
        azi += a2 - a1;
    }
    move = mat4to3(rotationMatrix(2, -azi)) * move;

    // apply the pan
    switch (camera->map->mapconfig->navigationSrsType())
    {
    case vtslibs::registry::Srs::Type::projected:
    {
        targetPosition += move;
    } break;
    case vtslibs::registry::Srs::Type::geographic:
    {
        double dist = length(vec3to2(move));
        double ang1 = radToDeg(atan2(move(0), move(1)));
        vec3 p = camera->map->convertor->geoDirect(
            targetPosition, dist, ang1);
        p(2) += move(2);

        // prevent the pan if it would cause unexpected direction change
        double r1 = angularDiff(position[0], p(0));
        double r2 = angularDiff(position[0], targetPosition[0]);
        if (std::abs(r1) < 30 || (r1 < 0) == (r2 < 0))
            targetPosition = p;
    } break;
    default:
        LOGTHROW(fatal, std::invalid_argument)
                << "Invalid navigation srs type";
    }

    assert(isNavigationModeValid());
}

void NavigationImpl::rotate(vec3 value)
{
    assert(isNavigationModeValid());

    if (camera->map->mapconfig->navigationSrsType()
            == vtslibs::registry::Srs::Type::geographic
            && options.navigationMode == NavigationMode::Dynamic)
        mode = NavigationMode::Free;

    targetOrientation += value.cwiseProduct(vec3(0.2, -0.1, 0.2)
                    * options.sensitivityRotate);

    assert(isNavigationModeValid());
}

void NavigationImpl::zoom(double value)
{
    assert(isNavigationModeValid());

    double c = value * options.sensitivityZoom * 120;
    targetVerticalExtent *= std::pow(1.002, -c);

    assert(isNavigationModeValid());
}

void NavigationImpl::resetNavigationMode()
{
    if (options.navigationMode == NavigationMode::Azimuthal
        || options.navigationMode == NavigationMode::Free)
        mode = options.navigationMode;
    else
        mode = NavigationMode::Azimuthal;
}

void NavigationImpl::convertSubjObj()
{
    vec3 center, dir, up;
    positionToCamera(center, dir, up, orientation, position);
    double dist = objectiveDistance();
    if (type == Type::objective)
        dist *= -1;
    center += dir * dist;
    position = targetPosition = camera->map->convertor->physToNav(center);
}

double NavigationImpl::objectiveDistance()
{
    return verticalExtent * 0.5 / tan(degToRad(verticalFov * 0.5));
}

void NavigationImpl::positionToCamera(vec3 &center, vec3 &dir, vec3 &up,
    const vec3 &inputRotation, const vec3 &inputPosition)
{
    // camera-space vectors
    vec3 rot = inputRotation;
    center = inputPosition;
    dir = vec3(1, 0, 0);
    up = vec3(0, 0, -1);

    // apply rotation
    {
        double yaw = camera->map->mapconfig->navigationSrsType()
            == vtslibs::registry::Srs::Type::projected
            ? rot(0) : -rot(0);
        mat3 tmp = mat4to3(rotationMatrix(2, yaw))
            * mat4to3(rotationMatrix(1, -rot(1)))
            * mat4to3(rotationMatrix(0, -rot(2)));
        dir = tmp * dir;
        up = tmp * up;
    }

    const auto &convertor = camera->map->convertor;

    // transform to physical srs
    switch (camera->map->mapconfig->navigationSrsType())
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
        dir = normalize(vec3(dir - center));
        up = normalize(vec3(up - center));
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
        n = normalize(vec3(n - center));
        e = normalize(vec3(e - center));
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

bool NavigationImpl::isNavigationModeValid() const
{
    if (mode != NavigationMode::Azimuthal
            && mode != NavigationMode::Free)
        return false;
    return true;
}

void NavigationImpl::setManual()
{
    suspendAltitudeChange = false;
    autoRotation = 0;
}

void NavigationImpl::setPosition(const vtslibs::registry::Position &position)
{
    autoRotation = 0;
    suspendAltitudeChange = true;
    positionAltitudeReset.reset();
    lastPositionAltitude.reset();
    heightMode = position.heightMode;
    type = position.type;
    verticalFov = position.verticalFov;
    targetVerticalExtent = position.verticalExtent;
    targetOrientation = vecFromUblas<vec3>(position.orientation);
    normalizeOrientation(targetOrientation);
    targetPosition = vecFromUblas<vec3>(position.position);
    resetNavigationMode();
}

vtslibs::registry::Position NavigationImpl::getPosition() const
{
    vtslibs::registry::Position res;
    res.type = type;
    res.heightMode = heightMode;
    res.position = vecToUblas<math::Point3>(position);
    res.orientation = vecToUblas<math::Point3>(orientation); // todo use camera ortientation
    res.verticalExtent = verticalExtent;
    res.verticalFov = verticalFov;
    return res;
}

void NavigationImpl::updateNavigation(double elapsedTime)
{
    OPTICK_EVENT();

    assert(options.inertiaPan >= 0
        && options.inertiaPan < 1);
    assert(options.inertiaRotate >= 0
        && options.inertiaRotate < 1);
    assert(options.inertiaZoom >= 0
        && options.inertiaZoom < 1);
    assert(options.navigationLatitudeThreshold > 0
        && options.navigationLatitudeThreshold < 90);

    MapImpl *map = camera->map;
    const auto &convertor = map->convertor;
    double majorRadius = map->body.majorRadius;

    vec3 &p = position;
    vec3 &r = orientation;
    vec3 &tp = targetPosition;
    vec3 &tr = targetOrientation;

    // convert floating position
    if (heightMode != HeightMode::fixed)
    {
        heightMode = HeightMode::fixed;
        positionAltitudeReset = position[2];
        position[2] = targetPosition[2] = 0;
    }

    // update navigation mode
    switch (options.navigationMode)
    {
    case NavigationMode::Azimuthal:
    case NavigationMode::Free:
        mode = options.navigationMode;
        break;
    case NavigationMode::Dynamic:
        if (map->mapconfig->navigationSrsType()
            == vtslibs::registry::Srs::Type::projected)
            mode = NavigationMode::Azimuthal;
        else if (std::abs(tp(1)) > options.navigationLatitudeThreshold - 1e-5)
            mode = NavigationMode::Free; // switch to free mode when too close to a pole
        break;
    case NavigationMode::Seamless:
        mode = verticalExtent
            < options.viewExtentThresholdScaleLow * majorRadius
            ? NavigationMode::Free : NavigationMode::Azimuthal;
        break;
    default:
        LOGTHROW(fatal, std::invalid_argument)
            << "Invalid navigation mode";
    }
    assert(isNavigationModeValid());

    // limit zoom
    if (options.cameraNormalization)
    {
        targetVerticalExtent = clamp(targetVerticalExtent,
            options.viewExtentLimitScaleMin * majorRadius,
            options.viewExtentLimitScaleMax * majorRadius);
    }

    // limit latitude in azimuthal navigation
    if (mode == NavigationMode::Azimuthal
        && map->mapconfig->navigationSrsType()
        == vtslibs::registry::Srs::Type::geographic)
    {
        tp(1) = clamp(tp(1),
            -options.navigationLatitudeThreshold,
            options.navigationLatitudeThreshold);
    }

    // auto rotation
    tr[0] += autoRotation;
    // todo autorotation fps compensation

    // limit yaw for seamless navigation mode
    if (options.cameraNormalization
        && options.navigationMode == NavigationMode::Seamless
        && mode == NavigationMode::Azimuthal
        && type == Type::objective)
        tr(0) = clamp(tr(0), -180 + 1e-7, 180 - 1e-7);
    else
        normalizeOrientation(tr);

    // limit tilt
    if (options.cameraNormalization
        && type == Type::objective)
    {
        tr(1) = clamp(tr(1), options.tiltLimitAngleLow,
            options.tiltLimitAngleHigh);
    }

    // camera normalization
    vec3 normalizedRotation = tr;
    if (options.cameraNormalization
        && type == Type::objective)
    {
        double &yaw = normalizedRotation(0);
        double &tilt = normalizedRotation(1);

        // limits by zoom
        {
            // find the interpolation factor
            double extCur = verticalExtent;
            double extLow = options.viewExtentThresholdScaleLow * majorRadius;
            double extHig = options.viewExtentThresholdScaleHigh * majorRadius;
            extCur = std::log2(extCur);
            extLow = std::log2(extLow);
            extHig = std::log2(extHig);
            double f = (extCur - extLow) / (extHig - extLow);
            f = clamp(f, 0, 1);
            f = smootherstep(f);

            // yaw limit
            if (options.navigationMode == NavigationMode::Azimuthal)
                yaw = 0;
            else if (options.navigationMode == NavigationMode::Seamless)
                yaw = interpolate(yaw, 0, f);

            // tilt limit
            tilt = interpolate(tilt, options.tiltLimitAngleLow, f);
        }

        // detect terrain obscurance
        {
            double ll = objectiveDistance();
            double sampleSize = verticalExtent
                / options.navigationSamplesPerViewExtent;
            double thresholdBase = verticalExtent * 0.15;
            for (double fraction = 0.4; fraction < 1.0; fraction += 0.02)
            {
                double l = ll * fraction;
                vec3 eye, forward, up;
                positionToCamera(eye, forward, up, normalizedRotation, p);
                eye -= forward * l;
                vec3 eyeNav = convertor->physToNav(eye);
                double altitude = nan1();
                if (camera->map->getSurfaceOverEllipsoid(altitude,
                    eyeNav, sampleSize * fraction))
                {
                    double threshold = thresholdBase * fraction;
                    altitude = eyeNav[2] - altitude;
                    if (altitude < threshold)
                    {
                        double a1 = std::sin(-degToRad(tilt)) * l;
                        double a2 = a1 + threshold - altitude;
                        double t = -radToDeg(std::asin(a2 / l));
                        if (!std::isnan(t))
                            tilt = t;
                    }
                }
            }
        }

        tilt = clamp(tilt, options.tiltLimitAngleLow,
            options.tiltLimitAngleHigh);
    }

    // navigation solver
    double azi1 = nan1();
    double azi2 = nan1();
    double horizontal1 = nan1();
    double horizontal2 = nan1();
    switch (map->mapconfig->navigationSrsType())
    {
    case vtslibs::registry::Srs::Type::projected:
        horizontal1 = length(vec2(
            vec3to2(tp) - vec3to2(p)));
        break;
    case vtslibs::registry::Srs::Type::geographic:
        map->convertor->geoInverse(p, tp, horizontal1,
            azi1, azi2);
        break;
    default:
        LOGTHROW(fatal, std::invalid_argument)
            << "Invalid navigation srs type";
    }
    double vertical2 = nan1();
    double prevExtent = verticalExtent;
    solveNavigation(
        options,
        elapsedTime,
        verticalFov,
        horizontal1,
        tp(2) - p(2),
        verticalExtent,
        targetVerticalExtent - verticalExtent,
        r,
        angularDiff(r, normalizedRotation),
        verticalExtent,
        horizontal2,
        vertical2,
        r
    );

    // horizontal move
    if (horizontal1 > 0)
    {
        switch (map->mapconfig->navigationSrsType())
        {
        case vtslibs::registry::Srs::Type::projected:
        {
            p += (tp - p) * (horizontal2 / horizontal1);
        } break;
        case vtslibs::registry::Srs::Type::geographic:
        {
            switch (mode)
            {
            case NavigationMode::Free:
            {
                p = map->convertor->geoDirect(p, horizontal2, azi1, azi2);
                r(0) += azi2 - azi1;
                tr(0) += azi2 - azi1;
            } break;
            case NavigationMode::Azimuthal:
            {
                for (int i = 0; i < 2; i++)
                    p(i) += angularDiff(p(i), tp(i))
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
        switch (map->mapconfig->navigationSrsType())
        {
        case vtslibs::registry::Srs::Type::projected:
        {
            const vtslibs::registry::Srs &srs =
                map->mapconfig->srs.get(
                    map->mapconfig->referenceFrame.model.navigationSrs);
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
            normalizeAngle(p(0));
        } break;
        default:
            LOGTHROW(fatal, std::invalid_argument)
                << "Invalid navigation srs type";
        }
        // reflect the same change in target position
        tp += p - pp;
    }

    // vertical move
    p(2) += vertical2;

    // altitude corrections
    if (options.cameraAltitudeChanges
        && !suspendAltitudeChange
        && type == Type::objective)
    {
        double f1 = horizontal2 / verticalExtent;
        double f2 = std::abs(std::log(verticalExtent) - std::log(prevExtent));
        double fadeOutFactor = std::max(f1, f2);
        if (!std::isnan(fadeOutFactor))
        {
            double surfaceOverEllipsoid = nan1();
            if (camera->map->getSurfaceOverEllipsoid(
                surfaceOverEllipsoid, targetPosition,
                verticalExtent / options.navigationSamplesPerViewExtent))
            {
                double &pa = targetPosition[2];
                if (positionAltitudeReset)
                {
                    pa = surfaceOverEllipsoid + *positionAltitudeReset;
                    positionAltitudeReset.reset();
                }
                else if (lastPositionAltitude)
                {
                    pa += surfaceOverEllipsoid - *lastPositionAltitude;
                    pa = interpolate(pa, surfaceOverEllipsoid,
                        std::min(1.0, fadeOutFactor)
                        * options.cameraAltitudeFadeOutFactor);
                    // todo fps compensation
                }
                else
                {
                    pa = surfaceOverEllipsoid;
                }
                lastPositionAltitude = surfaceOverEllipsoid;
            }
        }
    }

    // normalize rotation
    normalizeOrientation(r);

    // asserts
    assert(isNavigationModeValid());
    assert(r(0) >= -180 && r(0) <= 180);
    assert(r(1) >= -180 && r(1) <= 180);
    assert(r(2) >= -180 && r(2) <= 180);
    if (map->mapconfig->navigationSrsType()
        == vtslibs::registry::Srs::Type::geographic)
    {
        assert(p(0) >= -180 && p(0) <= 180);
        assert(p(1) >= -90 && p(1) <= 90);
    }

    // update the camera
    {
        Camera *cam = camera->camera;
        vec3 eye, forward, up, target;
        positionToCamera(eye, forward, up, r, p);
        if (type == Type::objective)
        {
            // objective position to subjective
            target = eye;
            eye -= forward * objectiveDistance();
        }
        else
            target = eye + forward;
        {
            double eye2[3], target2[3], up2[3];
            vecToRaw(eye, eye2);
            vecToRaw(target, target2);
            vecToRaw(up, up2);
            cam->setView(eye2, target2, up2);
        }
        {
            double n, f;
            cam->suggestedNearFar(n, f);
            cam->setProj(verticalFov, n, f);
        }
    }

    // render object position
    if (options.debugRenderObjectPosition)
    {
        vec3 phys = map->convertor->navToPhys(p);
        RenderSimpleTask r;
        r.mesh = map->getMesh("internal://data/meshes/cube.obj");
        r.mesh->priority = std::numeric_limits<float>::infinity();
        r.textureColor = map->getTexture(
            "internal://data/textures/helper.jpg");
        r.textureColor->priority = std::numeric_limits<float>::infinity();
        r.model = translationMatrix(phys)
            * scaleMatrix(verticalExtent * 0.015);
        if (r.ready())
            camera->draws.infographics.emplace_back(camera->convert(r));
    }

    // render target position
    if (options.debugRenderTargetPosition)
    {
        vec3 phys = map->convertor->navToPhys(tp);
        RenderSimpleTask r;
        r.mesh = map->getMesh("internal://data/meshes/cube.obj");
        r.mesh->priority = std::numeric_limits<float>::infinity();
        r.textureColor = map->getTexture(
            "internal://data/textures/helper.jpg");
        r.textureColor->priority = std::numeric_limits<float>::infinity();
        r.model = translationMatrix(phys)
            * scaleMatrix(targetVerticalExtent * 0.015);
        if (r.ready())
            camera->draws.infographics.emplace_back(camera->convert(r));
    }
}

void updateNavigation(std::weak_ptr<NavigationImpl> &nav, double elapsedTime)
{
    if (auto n = nav.lock())
        n->updateNavigation(elapsedTime);
}

void normalizeOrientation(vec3 &o)
{
    for (int i = 0; i < 3; i++)
        normalizeAngle(o[i]);
}

} // namespace vts
