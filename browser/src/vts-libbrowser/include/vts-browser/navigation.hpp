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

#ifndef NAVIGATION_HPP_jihsefk
#define NAVIGATION_HPP_jihsefk

#include <string>
#include <memory>
#include <vector>
#include <array>

#include "foundation.hpp"

namespace vts
{

class CameraImpl;
class Camera;
class NavigationOptions;
class NavigationImpl;
class Position;

class VTS_API Navigation
{
public:
    explicit Navigation(CameraImpl *cam);

    void pan(const double value[3]);
    void pan(const std::array<double, 3> &lst);
    void rotate(const double value[3]);
    void rotate(const std::array<double, 3> &lst);
    void zoom(double value);

    // corrects current position altitude to match the surface
    void resetAltitude();

    // this will reset navigation to azimuthal mode (or whichever is set)
    void resetNavigationMode();

    void setSubjective(bool subjective, bool convert);
    void setPoint(const double point[3]);
    void setPoint(const std::array<double, 3> &lst);
    void setRotation(const double rot[3]);
    void setRotation(const std::array<double, 3> &lst);
    void setViewExtent(double viewExtent);
    void setFov(double fov);
    void setAutoRotation(double value);
    void setPosition(const Position &position);

    bool getSubjective() const;
    void getPoint(double point[3]) const;
    void getRotation(double point[3]) const;
    double getViewExtent() const;
    double getFov() const;
    double getAutoRotation() const;
    Position getPosition() const;

    NavigationOptions &options();
    Camera *camera();

private:
    std::shared_ptr<NavigationImpl> impl;
    friend Camera;
};

} // namespace vts

#endif
