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

#include "navigation.hpp"
#include "piha.hpp"

namespace vts
{

NavigationImpl::NavigationImpl(CameraImpl *cam, Navigation *navigation) :
    camera(cam), navigation(navigation), changeRotation(0,0,0),
    targetPoint(0,0,0), autoRotation(0), targetViewExtent(0),
    mode(NavigationMode::Azimuthal), previousType(NavigationType::Quick)
{
    if (camera->map->mapconfig && *camera->map->mapconfig)
        initialize();
}

void NavigationImpl::initialize()
{
    assert(*camera->map->mapconfig);

    position = camera->map->mapconfig->position;
    targetPoint = vecFromUblas<vec3>(position.position);
    changeRotation = vec3(0, 0, 0);
    targetViewExtent = position.verticalExtent;
    autoRotation = camera->map->mapconfig->browserOptions.autorotate;
    for (int i = 0; i < 3; i++)
        normalizeAngle(position.orientation[i]);

    assert(isNavigationModeValid());
}

uint32 NavigationImpl::applyCameraRotationNormalization(vec3 &rot)
{
    if (!options.cameraNormalization
            || options.navigationType == NavigationType::FlyOver
            || position.type == vtslibs::registry::Position::Type::subjective)
        return 0;

    // find the interpolation factor
    double extCur = position.verticalExtent;
    double majorRadius = camera->map->body.majorRadius;
    double extLow = options.viewExtentThresholdScaleLow * majorRadius;
    double extHig = options.viewExtentThresholdScaleHigh * majorRadius;
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

uint32 NavigationImpl::applyCameraRotationNormalization(math::Point3 &rot)
{
    vec3 r = vecFromUblas<vec3>(rot);
    auto res = applyCameraRotationNormalization(r);
    rot = vecToUblas<math::Point3>(r);
    return res;
}

void NavigationImpl::resetNavigationMode()
{
    if (options.navigationMode == NavigationMode::Azimuthal
            || options.navigationMode == NavigationMode::Free)
        mode = options.navigationMode;
    else
        mode = NavigationMode::Azimuthal;
}

void NavigationImpl::convertPositionSubjObj()
{
    vec3 center, dir, up;
    positionToCamera(center, dir, up);
    double dist = positionObjectiveDistance();
    if (position.type == vtslibs::registry::Position::Type::objective)
        dist *= -1;
    center += dir * dist;
    position.position = vecToUblas<math::Point3>(
        camera->map->convertor->physToNav(center));
    targetPoint = vecFromUblas<vec3>(position.position);
}

void NavigationImpl::positionToCamera(vec3 &center, vec3 &dir, vec3 &up)
{
    // camera-space vectors
    vec3 rot = vecFromUblas<vec3>(position.orientation);
    applyCameraRotationNormalization(rot);
    center = vecFromUblas<vec3>(position.position);
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

double NavigationImpl::positionObjectiveDistance()
{
    return position.verticalExtent
            * 0.5 / tan(degToRad(position.verticalFov * 0.5));
}

void NavigationImpl::navigationTypeChanged()
{
    if (previousType != options.navigationType)
    {
        // fly over must first apply the old camera limits
        std::swap(previousType, options.navigationType);
        applyCameraRotationNormalization(position.orientation);

        // store the new value
        options.navigationType = previousType;
    }
}

void NavigationImpl::updateNavigation(double elapsedTime)
{
    assert(options.inertiaPan >= 0
            && options.inertiaPan < 1);
    assert(options.inertiaRotate >= 0
            && options.inertiaRotate < 1);
    assert(options.inertiaZoom >= 0
            && options.inertiaZoom < 1);
    assert(options.navigationLatitudeThreshold > 0
           && options.navigationLatitudeThreshold < 90);

    MapImpl *map = camera->map;

    // convert floating position
    if (position.heightMode != vtslibs::registry::Position::HeightMode::fixed)
    {
        position.heightMode = vtslibs::registry::Position::HeightMode::fixed;
        positionAltitudeReset.emplace(position.position[2]);
        position.position[2] = 0;
    }

    // check if navigation type has changed
    navigationTypeChanged();

    vec3 p = vecFromUblas<vec3>(position.position);
    vec3 r = vecFromUblas<vec3>(position.orientation);

    // limit zoom
    double majorRadius = map->body.majorRadius;
    targetViewExtent = clamp(targetViewExtent,
           options.viewExtentLimitScaleMin * majorRadius,
           options.viewExtentLimitScaleMax * majorRadius);

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
        else
        {
            // switch to free mode when too close to a pole
            if (std::abs(targetPoint(1))
                    > options.navigationLatitudeThreshold - 1e-5)
                mode = NavigationMode::Free;
        }
        break;
    case NavigationMode::Seamless:
        mode = position.verticalExtent
                < options.viewExtentThresholdScaleLow * majorRadius
                ? NavigationMode::Free : NavigationMode::Azimuthal;
        break;
    default:
        LOGTHROW(fatal, std::invalid_argument)
                << "Invalid navigation mode";
    }

    // limit latitude in azimuthal navigation
    if (mode == NavigationMode::Azimuthal
            && map->mapconfig->navigationSrsType()
                    != vtslibs::registry::Srs::Type::projected)
    {
        targetPoint(1) = clamp(targetPoint(1),
                -options.navigationLatitudeThreshold,
                options.navigationLatitudeThreshold);
    }
    assert(isNavigationModeValid());

    // auto rotation
    changeRotation(0) += autoRotation;

    // limit rotation for seamless navigation mode
    if (options.cameraNormalization
            && options.navigationMode == NavigationMode::Seamless
            && mode == NavigationMode::Azimuthal
            && position.type
                    == vtslibs::registry::Position::Type::objective)
    {
        double yaw = position.orientation[0];
        double &ch = changeRotation(0);
        ch = clamp(yaw + ch, -180 + 1e-7, 180 - 1e-7) - yaw;
        assert(yaw + ch >= -180 && yaw + ch <= 180);
    }

    // find inputs for perceptually invariant motion
    double azi1 = std::numeric_limits<double>::quiet_NaN();
    double azi2 = std::numeric_limits<double>::quiet_NaN();
    double horizontal1 = std::numeric_limits<double>::quiet_NaN();
    double horizontal2 = std::numeric_limits<double>::quiet_NaN();
    switch (map->mapconfig->navigationSrsType())
    {
    case vtslibs::registry::Srs::Type::projected:
        horizontal1 = length(vec2(
                        vec3to2(targetPoint) - vec3to2(p)));
        break;
    case vtslibs::registry::Srs::Type::geographic:
        map->convertor->geoInverse(p, targetPoint, horizontal1,
                              azi1, azi2);
        break;
    default:
        LOGTHROW(fatal, std::invalid_argument)
                << "Invalid navigation srs type";
    }
    double vertical1 = targetPoint(2) - p(2);
    double vertical2 = std::numeric_limits<double>::quiet_NaN();
    vec3 r2(vertical2, vertical2, vertical2);
    double prevExtent = position.verticalExtent;
    navigationPiha(
                options,
                elapsedTime,
                position.verticalFov,
                horizontal1,
                vertical1,
                prevExtent,
                targetViewExtent - position.verticalExtent,
                r,
                changeRotation,
                position.verticalExtent,
                horizontal2,
                vertical2,
                r2
                );

    // vertical move
    p(2) += vertical2;

    // rotation
    changeRotation -= r2 - r;
    r = r2;

    // horizontal move
    if (horizontal1 > 0)
    {
        switch (map->mapconfig->navigationSrsType())
        {
        case vtslibs::registry::Srs::Type::projected:
        {
            p += (targetPoint - p) * (horizontal2 / horizontal1);
        } break;
        case vtslibs::registry::Srs::Type::geographic:
        {
            switch (mode)
            {
            case NavigationMode::Free:
            {
                p = map->convertor->geoDirect(p, horizontal2, azi1, azi2);
                r(0) += azi2 - azi1;
            } break;
            case NavigationMode::Azimuthal:
            {
                for (int i = 0; i < 2; i++)
                    p(i) += angularDiff(p(i), targetPoint(i))
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
            p(0) = modulo(p(0) + 180, 360) - 180;
        } break;
        default:
            LOGTHROW(fatal, std::invalid_argument)
                    << "Invalid navigation srs type";
        }
        targetPoint += p - pp;
    }

    // normalize rotation
    if (options.cameraNormalization
            && position.type == vtslibs::registry::Position::Type::objective
            && position.type
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
    if (map->mapconfig->navigationSrsType()
            == vtslibs::registry::Srs::Type::geographic)
    {
        assert(p(0) >= -180 && p(0) <= 180);
        assert(p(1) >= -90 && p(1) <= 90);
    }

    // store changed values
    position.position = vecToUblas<math::Point3>(p);
    position.orientation = vecToUblas<math::Point3>(r);

    // altitude corrections
    if (position.type == vtslibs::registry::Position::Type::objective)
    {
        double f1 = horizontal2 / position.verticalExtent;
        double f2 = std::abs(std::log(position.verticalExtent)
                              - std::log(prevExtent));
        updatePositionAltitude(std::max(f1, f2));
    }

    // update the camera
    {
        Camera *cam = camera->camera;
        vec3 eye, forward, up, target;
        positionToCamera(eye, forward, up);
        if (position.type == vtslibs::registry::Position::Type::objective)
        {
            // objective position to subjective
            target = eye;
            eye = eye - forward * positionObjectiveDistance();
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
            cam->setProj(position.verticalFov, n, f);
        }
    }

    // render object position
    if (options.debugRenderObjectPosition)
    {
        vec3 phys = map->convertor->navToPhys(vecFromUblas<vec3>(position.position));
        RenderSimpleTask r;
        r.mesh = map->getMesh("internal://data/meshes/cube.obj");
        r.mesh->priority = std::numeric_limits<float>::infinity();
        r.textureColor = map->getTexture("internal://data/textures/helper.jpg");
        r.textureColor->priority = std::numeric_limits<float>::infinity();
        r.model = translationMatrix(phys)
            * scaleMatrix(position.verticalExtent * 0.015);
        if (r.ready())
            camera->draws.infographics.emplace_back(camera->convert(r));
    }

    // render target position
    if (options.debugRenderTargetPosition)
    {
        vec3 phys = map->convertor->navToPhys(targetPoint);
        RenderSimpleTask r;
        r.mesh = map->getMesh("internal://data/meshes/cube.obj");
        r.mesh->priority = std::numeric_limits<float>::infinity();
        r.textureColor = map->getTexture("internal://data/textures/helper.jpg");
        r.textureColor->priority = std::numeric_limits<float>::infinity();
        r.model = translationMatrix(phys)
            * scaleMatrix(targetViewExtent * 0.015);
        if (r.ready())
            camera->draws.infographics.emplace_back(camera->convert(r));
    }
}

bool NavigationImpl::isNavigationModeValid() const
{
    if (mode != NavigationMode::Azimuthal
            && mode != NavigationMode::Free)
        return false;
    return true;
}

vec3 NavigationImpl::applyCameraRotationNormalizationPermanently()
{
    vec3 posRot = vecFromUblas<vec3>(position.orientation);
    if (applyCameraRotationNormalization(posRot) == 2)
    {
        position.orientation = vecToUblas<math::Point3>(posRot);
        if (mode == NavigationMode::Azimuthal)
            changeRotation = vec3(0,0,0);
    }
    return posRot;
}

void NavigationImpl::pan(vec3 value)
{
    assert(isNavigationModeValid());

    vec3 posRot = applyCameraRotationNormalizationPermanently();

    // camera roll
    value = mat4to3(rotationMatrix(2, -posRot[2])) * value;

    // slower movement near poles
    double h = 1;
    if (camera->map->mapconfig->navigationSrsType()
            == vtslibs::registry::Srs::Type::geographic
            && mode == NavigationMode::Azimuthal)
    {
        h = std::cos(degToRad(position.position[1]));
    }

    // pan speed depends on zoom level
    double v = position.verticalExtent / 600;
    vec3 move = value.cwiseProduct(vec3(-2 * v * h, 2 * v, 2)
                    * options.sensitivityPan);

    // compute change of azimuth
    double azi = posRot[0];
    if (camera->map->mapconfig->navigationSrsType()
                == vtslibs::registry::Srs::Type::geographic)
    {
        // camera rotation taken from previous target position
        // this prevents strange turning near poles
        double d, a1, a2;
        camera->map->convertor->geoInverse(
                    vecFromUblas<vec3>(position.position),
                    targetPoint, d, a1, a2);
        azi += a2 - a1;
    }
    move = mat4to3(rotationMatrix(2, -azi)) * move;

    // apply the pan
    switch (camera->map->mapconfig->navigationSrsType())
    {
    case vtslibs::registry::Srs::Type::projected:
    {
        targetPoint += move;
    } break;
    case vtslibs::registry::Srs::Type::geographic:
    {
        vec3 p = targetPoint;
        double ang1 = radToDeg(atan2(move(0), move(1)));
        double dist = length(vec3to2(move));
        p = camera->map->convertor->geoDirect(p, dist, ang1);
        p(2) += move(2);

        // prevent the pan if it would cause unexpected direction change
        double r1 = angularDiff(position.position[0], p(0));
        double r2 = angularDiff(position.position[0], targetPoint[0]);
        if (std::abs(r1) < 30 || (r1 < 0) == (r2 < 0))
            targetPoint = p;
    } break;
    default:
        LOGTHROW(fatal, std::invalid_argument)
                << "Invalid navigation srs type";
    }

    autoRotation = 0;

    assert(isNavigationModeValid());
}

void NavigationImpl::rotate(vec3 value)
{
    assert(isNavigationModeValid());

    if (camera->map->mapconfig->navigationSrsType()
            == vtslibs::registry::Srs::Type::geographic
            && options.navigationMode == NavigationMode::Dynamic)
        mode = NavigationMode::Free;

    changeRotation += value.cwiseProduct(vec3(0.2, -0.1, 0.2)
                    * options.sensitivityRotate);
    applyCameraRotationNormalizationPermanently();
    autoRotation = 0;

    assert(isNavigationModeValid());
}

void NavigationImpl::zoom(double value)
{
    assert(isNavigationModeValid());

    double c = value * options.sensitivityZoom * 120;
    targetViewExtent *= std::pow(1.002, -c);
    autoRotation = 0;

    assert(isNavigationModeValid());
}

void NavigationImpl::setPoint(const vec3 &point)
{
    assert(isNavigationModeValid());

    targetPoint = point;
    autoRotation = 0;
    if (options.navigationType == NavigationType::Instant)
        lastPositionAltitude.reset();
}

void NavigationImpl::setRotation(const vec3 &euler)
{
    assert(isNavigationModeValid());

    changeRotation = angularDiff(
        vecFromUblas<vec3>(position.orientation), euler);
    autoRotation = 0;
}

void NavigationImpl::setViewExtent(double viewExtent)
{
    assert(isNavigationModeValid());

    targetViewExtent = viewExtent;
    autoRotation = 0;
}

void updateNavigation(std::weak_ptr<NavigationImpl> nav, double elapsedTime)
{
    auto n = nav.lock();
    if (n)
        n->updateNavigation(elapsedTime);
}

} // namespace vts
