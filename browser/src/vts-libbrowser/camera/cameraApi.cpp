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

#include "camera.hpp"
#include "../include/vts-browser/map.hpp"

namespace vts
{

std::shared_ptr<Camera> Map::camera()
{
    auto r = std::make_shared<Camera>(impl.get());
    impl->cameras.push_back(r->impl);
    return r;
}

Camera::Camera(MapImpl *map)
{
    impl = std::make_shared<CameraImpl>(map, this);
}

void Camera::setViewportSize(uint32 width, uint32 height)
{
    impl->windowWidth = width;
    impl->windowHeight = height;
}

void Camera::setView(const double eye[3], const double target[3],
    const double up[3])
{
    impl->eye = rawToVec3(eye);
    impl->target = rawToVec3(target);
    impl->up = rawToVec3(up);
}

void Camera::setView(const std::array<double, 3> &eye,
    const std::array<double, 3> &target,
    const std::array<double, 3> &up)
{
    setView(eye.data(), target.data(), up.data());
}

void Camera::setProj(double fovyDegs, double near_, double far_)
{
    impl->fovyDegs = fovyDegs;
    impl->near_ = near_;
    impl->far_ = far_;
}

void Camera::getViewportSize(uint32 &width, uint32 &height)
{
    width = impl->windowWidth;
    height = impl->windowHeight;
}

void Camera::getView(double eye[3], double target[3], double up[3])
{
    vecToRaw(impl->eye, eye);
    vecToRaw(impl->target, target);
    vecToRaw(impl->up, up);
}

void Camera::getProj(double &fovyDegs, double &near_, double &far_)
{
    fovyDegs = impl->fovyDegs;
    near_ = impl->near_;
    far_ = impl->far_;
}

void Camera::suggestedNearFar(double &near_, double &far_)
{
    impl->suggestedNearFar(near_, far_);
}

CameraStatistics &Camera::statistics()
{
    return impl->statistics;
}

CameraOptions &Camera::options()
{
    return impl->options;
}

CameraDraws &Camera::draws()
{
    return impl->draws;
}

CameraCredits &Camera::credits()
{
    return impl->credits;
}

Map *Camera::map()
{
    return impl->map->map;
}

} // namespace vts
