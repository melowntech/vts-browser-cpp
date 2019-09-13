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

#include "../include/vts-browser/camera.hpp"
#include "../include/vts-browser/navigation.hpp"
#include "../include/vts-browser/position.hpp"

#include "../navigation.hpp"
#include "../camera.hpp"
#include "../mapConfig.hpp"
#include "../map.hpp"
#include "../position.hpp"

namespace vts
{

std::shared_ptr<Navigation> Camera::createNavigation()
{
    auto r = std::make_shared<Navigation>(impl.get());
    impl->navigation = r->impl;
    return r;
}

Navigation::Navigation(CameraImpl *cam)
{
    assert(cam);
    impl = std::make_shared<NavigationImpl>(cam, this);
}

void Navigation::pan(const double value[3])
{
    if (!impl->camera->map->mapconfigReady)
        return;
    impl->setManual();
    if (impl->type == NavigationImpl::Type::objective)
        impl->pan(rawToVec3(value));
    else
        impl->rotate(-rawToVec3(value));
}

void Navigation::pan(const std::array<double, 3> &lst)
{
    pan(lst.data());
}

void Navigation::setPoint(const double point[3])
{
    if (!impl->camera->map->mapconfigAvailable)
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    assert(!std::isnan(point[0]));
    assert(!std::isnan(point[1]));
    assert(!std::isnan(point[2]));
    if (impl->camera->map->mapconfig->navigationSrsType()
                   == vtslibs::registry::Srs::Type::geographic)
    {
        assert(point[0] >= -180 && point[0] <= 180);
        assert(point[1] >= -90 && point[1] <= 90);
    }
    impl->setManual();
    if (impl->options.type == NavigationType::Instant)
        impl->lastPositionAltitude.reset();
    impl->targetPosition = rawToVec3(point);
}

void Navigation::setPoint(const std::array<double, 3> &lst)
{
    setPoint(lst.data());
}

void Navigation::rotate(const double value[3])
{
    if (!impl->camera->map->mapconfigReady)
        return;
    impl->setManual();
    impl->rotate(rawToVec3(value));
}

void Navigation::rotate(const std::array<double, 3> &lst)
{
    rotate(lst.data());
}

void Navigation::setRotation(const double point[3])
{
    if (!impl->camera->map->mapconfigAvailable)
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    assert(!std::isnan(point[0]));
    assert(!std::isnan(point[1]));
    assert(!std::isnan(point[2]));
    impl->setManual();
    impl->targetOrientation = rawToVec3(point);
    normalizeOrientation(impl->targetOrientation);
}

void Navigation::setRotation(const std::array<double, 3> &lst)
{
    setRotation(lst.data());
}

void Navigation::setAutoRotation(double value)
{
    if (!impl->camera->map->mapconfigAvailable)
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    impl->autoRotation = value;
    impl->suspendAltitudeChange = true;
}

void Navigation::setPosition(const Position &p)
{
    if (!impl->camera->map->mapconfigAvailable)
    {
        LOGTHROW(err4, std::logic_error)
            << "Map is not yet available.";
    }
    impl->setPosition(p2p(p));
}

void Navigation::zoom(double value)
{
    if (!impl->camera->map->mapconfigReady)
        return;
    impl->setManual();
    if (impl->type == NavigationImpl::Type::objective)
        impl->zoom(value);
}

void Navigation::setViewExtent(double viewExtent)
{
    if (!impl->camera->map->mapconfigAvailable)
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    assert(!std::isnan(viewExtent) && viewExtent > 0);
    impl->setManual();
    impl->targetVerticalExtent = viewExtent;
}

void Navigation::setFov(double fov)
{
    if (!impl->camera->map->mapconfigAvailable)
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    assert(fov > 0 && fov < 180);
    impl->setManual();
    impl->verticalFov = fov;
}

void Navigation::setSubjective(bool subjective, bool convert)
{
    if (!impl->camera->map->mapconfigAvailable)
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    if (subjective == getSubjective())
        return;
    if (convert)
        impl->convertSubjObj();
    impl->type = subjective
            ? NavigationImpl::Type::subjective
            : NavigationImpl::Type::objective;
    impl->setManual();
}

void Navigation::resetAltitude()
{
    if (!impl->camera->map->mapconfigAvailable)
    {
        LOGTHROW(err4, std::logic_error)
            << "Map is not yet available.";
    }
    impl->setManual();
    impl->positionAltitudeReset = 0;
}

void Navigation::resetNavigationMode()
{
    if (!impl->camera->map->mapconfigAvailable)
    {
        LOGTHROW(err4, std::logic_error)
            << "Map is not yet available.";
    }
    impl->setManual();
    impl->resetNavigationMode();
}

void Navigation::getPoint(double point[3]) const
{
    if (!impl->camera->map->mapconfigAvailable)
        return vecToRaw(vec3(0, 0, 0), point);
    vecToRaw(impl->position, point);
}

void Navigation::getRotation(double point[3]) const
{
    if (!impl->camera->map->mapconfigAvailable)
        return vecToRaw(vec3(0, 0, 0), point);
    vecToRaw(impl->orientation, point);
}

double Navigation::getAutoRotation() const
{
    if (!impl->camera->map->mapconfigAvailable)
        return 0;
    return impl->autoRotation;
}

Position Navigation::getPosition() const
{
    if (!impl->camera->map->mapconfigAvailable)
        return {};
    return p2p(impl->getPosition());
}

double Navigation::getViewExtent() const
{
    if (!impl->camera->map->mapconfigAvailable)
        return 0;
    return impl->verticalExtent;
}

double Navigation::getFov() const
{
    if (!impl->camera->map->mapconfigAvailable)
        return 0;
    return impl->verticalFov;
}

bool Navigation::getSubjective() const
{
    if (!impl->camera->map->mapconfigAvailable)
        return false;
    return impl->type == NavigationImpl::Type::subjective;
}

NavigationOptions &Navigation::options()
{
    return impl->options;
}

Camera *Navigation::camera()
{
    return impl->camera->camera;
}

} // namespace vts
