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

#ifndef NAVIGATION_HPP_sdgr7h8tz9jl9
#define NAVIGATION_HPP_sdgr7h8tz9jl9

#include "../camera/camera.hpp"

#include "../include/vts-browser/navigation.hpp"
#include "../include/vts-browser/navigationOptions.hpp"

namespace vts
{

class NavigationImpl
{
public:

    CameraImpl *const camera;
    Navigation *const navigation;
    NavigationOptions options;
    vtslibs::registry::Position position;
    vec3 changeRotation;
    vec3 targetPoint;
    double autoRotation;
    double targetViewExtent;
    boost::optional<double> lastPositionAltitude;
    boost::optional<double> positionAltitudeReset;
    NavigationMode mode;
    NavigationType previousType;

    NavigationImpl(CameraImpl *cam, Navigation *navigation);

    void initialize();
    void pan(vec3 value);
    void rotate(vec3 value);
    void zoom(double value);
    void setPoint(const vec3 &point);
    void setRotation(const vec3 &euler);
    void setViewExtent(double viewExtent);
    void updatePositionAltitude(double fadeOutFactor
                = std::numeric_limits<double>::quiet_NaN());
    void resetNavigationMode();
    void convertPositionSubjObj();
    void positionToCamera(vec3 &center, vec3 &dir, vec3 &up);
    double positionObjectiveDistance();
    void navigationTypeChanged();
    void updateNavigation(double elapsedTime);
    bool isNavigationModeValid() const;
    uint32 applyCameraRotationNormalization(vec3 &rot);
    uint32 applyCameraRotationNormalization(math::Point3 &rot);
    vec3 applyCameraRotationNormalizationPermanently();
};

} // namespace vts

#endif
