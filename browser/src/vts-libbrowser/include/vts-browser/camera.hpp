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

#ifndef CAMERA_HPP_jihsefk
#define CAMERA_HPP_jihsefk

#include <array>
#include <memory>

#include "foundation.hpp"

namespace vts
{

class MapImpl;
class CameraCredits;
class CameraDraws;
class CameraOptions;
class CameraStatistics;
class Map;
class Navigation;
class CameraImpl;

class VTS_API Camera
{
public:
    explicit Camera(MapImpl *map);

    void setViewportSize(uint32 width, uint32 height);
    void setView(const double eye[3], const double target[3],
                const double up[3]);
    void setView(const std::array<double, 3> &eye,
                const std::array<double, 3> &target,
                const std::array<double, 3> &up);
    void setView(const double view[16]);
    void setView(const std::array<double, 16> &view);
    void setProj(double fovyDegs, double near_, double far_);
    void setProj(const double proj[16]);

    void getViewportSize(uint32 &width, uint32 &height);
    void getView(double eye[3], double target[3], double up[3]);
    void getView(double view[16]);
    void getProj(double proj[16]);

    void suggestedNearFar(double &near_, double &far_);

    void renderUpdate();

    CameraCredits &credits();
    CameraDraws &draws();
    CameraOptions &options();
    CameraStatistics &statistics();
    Map *map();

    // create new navigation
    // only the last navigation created for this camera will be used
    std::shared_ptr<Navigation> createNavigation();

private:
    std::shared_ptr<CameraImpl> impl;
    friend Map;
};

} // namespace vts

#endif
