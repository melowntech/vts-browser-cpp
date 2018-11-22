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

#include <vts-libs/registry/json.hpp>
#include <vts-libs/registry/io.hpp>
#include <boost/lexical_cast.hpp>

#include "../include/vts-browser/map.hpp"
#include "navigation.hpp"
#include "../utilities/json.hpp"

namespace vts
{

std::shared_ptr<Navigation> Camera::navigation()
{
    auto r = std::make_shared<Navigation>(impl.get());
    impl->navigation = r->impl;
    return r;
}

Navigation::Navigation(CameraImpl *cam)
{
    impl = std::make_shared<NavigationImpl>(cam, this);
}

void Navigation::pan(const double value[3])
{
    if (!impl->camera->map->map->getMapconfigReady())
        return;
    if (impl->position.type
            == vtslibs::registry::Position::Type::objective)
        impl->pan(rawToVec3(value));
    else
        impl->rotate(-rawToVec3(value));
}

void Navigation::pan(const std::array<double, 3> &lst)
{
    pan(lst.data());
}

void Navigation::rotate(const double value[3])
{
    if (!impl->camera->map->map->getMapconfigReady())
        return;
    impl->rotate(rawToVec3(value));
}

void Navigation::rotate(const std::array<double, 3> &lst)
{
    rotate(lst.data());
}

void Navigation::zoom(double value)
{
    if (!impl->camera->map->map->getMapconfigReady())
        return;
    if (impl->position.type
            == vtslibs::registry::Position::Type::objective)
        impl->zoom(value);
}

void Navigation::resetAltitude()
{
    if (!impl->camera->map->map->getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
            << "Map is not yet available.";
    }
    impl->positionAltitudeReset = 0;
    impl->updatePositionAltitude();
}

void Navigation::resetNavigationMode()
{
    if (!impl->camera->map->map->getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
            << "Map is not yet available.";
    }
    impl->resetNavigationMode();
}

void Navigation::setSubjective(bool subjective, bool convert)
{
    if (!impl->camera->map->map->getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    if (subjective == getSubjective())
        return;
    if (convert)
        impl->convertPositionSubjObj();
    impl->position.type = subjective
            ? vtslibs::registry::Position::Type::subjective
            : vtslibs::registry::Position::Type::objective;
}

bool Navigation::getSubjective() const
{
    if (!impl->camera->map->map->getMapconfigAvailable())
        return false;
    return impl->position.type
            == vtslibs::registry::Position::Type::subjective;
}

void Navigation::setPoint(const double point[3])
{
    if (!impl->camera->map->map->getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    assert(point[0] == point[0]);
    assert(point[1] == point[1]);
    assert(point[2] == point[2]);
    if (impl->camera->map->mapconfig->navigationSrsType()
                   == vtslibs::registry::Srs::Type::geographic)
    {
        assert(point[0] >= -180 && point[0] <= 180);
        assert(point[1] >= -90 && point[1] <= 90);
    }
    vec3 v = rawToVec3(point);
    impl->setPoint(v);
}

void Navigation::setPoint(const std::array<double, 3> &lst)
{
    setPoint(lst.data());
}

void Navigation::getPoint(double point[3]) const
{
    if (!impl->camera->map->map->getMapconfigAvailable())
    {
        for (int i = 0; i < 3; i++)
            point[i] = 0;
        return;
    }
    vecToRaw(vecFromUblas<vec3>(impl->position.position), point);
}

void Navigation::setRotation(const double point[3])
{
    if (!impl->camera->map->map->getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    assert(point[0] == point[0]);
    assert(point[1] == point[1]);
    assert(point[2] == point[2]);
    impl->setRotation(rawToVec3(point));
}

void Navigation::setRotation(const std::array<double, 3> &lst)
{
    setRotation(lst.data());
}

void Navigation::getRotation(double point[3]) const
{
    if (!impl->camera->map->map->getMapconfigAvailable())
    {
        for (int i = 0; i < 3; i++)
            point[i] = 0;
        return;
    }
    vecToRaw(vecFromUblas<vec3>(impl->position.orientation), point);
}

void Navigation::getRotationLimited(double point[3]) const
{
    if (!impl->camera->map->map->getMapconfigAvailable())
    {
        for (int i = 0; i < 3; i++)
            point[i] = 0;
        return;
    }
    vec3 p = vecFromUblas<vec3>(impl->position.orientation);
    impl->applyCameraRotationNormalization(p);
    vecToRaw(p, point);
}

void Navigation::setViewExtent(double viewExtent)
{
    if (!impl->camera->map->map->getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    assert(viewExtent == viewExtent && viewExtent > 0);
    impl->setViewExtent(viewExtent);
}

double Navigation::getViewExtent() const
{
    if (!impl->camera->map->map->getMapconfigAvailable())
        return 0;
    return impl->position.verticalExtent;
}

void Navigation::setFov(double fov)
{
    if (!impl->camera->map->map->getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    assert(fov > 0 && fov < 180);
    impl->position.verticalFov = fov;
}

double Navigation::getFov() const
{
    if (!impl->camera->map->map->getMapconfigAvailable())
        return 0;
    return impl->position.verticalFov;
}

std::string Navigation::getPositionJson() const
{
    if (!impl->camera->map->map->getMapconfigAvailable())
        return "";
    vtslibs::registry::Position p = impl->position;
    impl->applyCameraRotationNormalization(p.orientation);
    return jsonToString(vtslibs::registry::asJson(p));
}

std::string Navigation::getPositionUrl() const
{
    if (!impl->camera->map->map->getMapconfigAvailable())
        return "";
    vtslibs::registry::Position p = impl->position;
    impl->applyCameraRotationNormalization(p.orientation);
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(10);
    ss << p;
    return ss.str();
}

namespace
{

void setPosition(Navigation *nav, NavigationImpl *impl,
                 const vtslibs::registry::Position &position)
{
    impl->navigationTypeChanged();
    impl->position.heightMode = position.heightMode;
    nav->setSubjective(
            position.type == vtslibs::registry::Position::Type::subjective, false);
    nav->setFov(position.verticalFov);
    nav->setViewExtent(position.verticalExtent);
    vec3 v = vecFromUblas<vec3>(position.orientation);
    nav->setRotation(v.data());
    v = vecFromUblas<vec3>(position.position);
    nav->setPoint(v.data());
}

} // namespace

void Navigation::setPositionJson(const std::string &position)
{
    if (!impl->camera->map->map->getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    Json::Value val = stringToJson(position);
    vtslibs::registry::Position pos = vtslibs::registry::positionFromJson(val);
    setPosition(this, impl.get(), pos);
}

void Navigation::setPositionUrl(const std::string &position)
{
    if (!impl->camera->map->map->getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    vtslibs::registry::Position pos;
    try
    {
        pos = boost::lexical_cast<vtslibs::registry::Position>(position);
    }
    catch(...)
    {
        LOGTHROW(err2, std::runtime_error) << "Invalid position from url.";
    }
    setPosition(this, impl.get(), pos);
}

void Navigation::setAutoRotation(double value)
{
    if (!impl->camera->map->map->getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    impl->autoRotation = value;
}

double Navigation::getAutoRotation() const
{
    if (!impl->camera->map->map->getMapconfigAvailable())
        return 0;
    return impl->autoRotation;
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
