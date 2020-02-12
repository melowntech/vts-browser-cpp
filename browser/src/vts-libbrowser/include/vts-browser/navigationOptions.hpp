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

#ifndef NAVIGATION_OPTIONS_HPP_kwegfdzvgsdfj
#define NAVIGATION_OPTIONS_HPP_kwegfdzvgsdfj

#include <string>

#include "foundation.hpp"

namespace vts
{

class VTS_API NavigationOptions
{
public:
    NavigationOptions();
    explicit NavigationOptions(const std::string &json);
    void applyJson(const std::string &json);
    std::string toJson() const;

    // multiplier applied to mouse input for the respective actions
    double sensitivityPan = 1;
    double sensitivityZoom = 1;
    double sensitivityRotate = 1;

    // inertia coefficients [0 - 1) for smoothing the navigation changes
    // inertia of zero makes all changes to apply immediately
    // while inertia of almost one makes the moves very smooth and slow
    double inertiaPan = 0.9;
    double inertiaZoom = 0.9;
    double inertiaRotate = 0.9;

    // lower and upper limit for view-extent
    // expressed as multiplicative factor of planet major radius
    double viewExtentLimitScaleMin;
    double viewExtentLimitScaleMax;

    // view-extent thresholds at which the camera normalization takes effect
    // expressed as multiplicative factor of planet major radius
    double viewExtentThresholdScaleLow;
    double viewExtentThresholdScaleHigh;

    // camera tilt limits (eg. -90 - 0)
    double tiltLimitAngleLow = -90;
    double tiltLimitAngleHigh = -10;

    // multiplicative factor at which camera altitude will converge to terrain
    //   when panning or zooming
    // range 0 (off) to 1 (fast)
    double altitudeFadeOutFactor = 0.5;

    // latitude threshold (0 - 90) used for azimuthal navigation
    double azimuthalLatitudeThreshold = 80;

    // fly over motion trajectory shape parameter
    // low -> flat trajectory
    // high -> spiky trajectory
    double flyOverSpikinessFactor = 2.5;

    // speed configuration for fly over navigation type
    double flyOverMotionChangeFraction = 0.5;
    double flyOverRotationChangeSpeed = 0.5;

    NavigationType type = NavigationType::Quick;
    NavigationMode mode = NavigationMode::Seamless;

    // limits camera tilt and yaw
    // uses viewExtentThresholdScaleLow/High
    // applies tiltLimitAngleLow/High (and yaw limit)
    bool enableNormalization = true;

    // vertically converges objective position towards ground
    bool enableAltitudeCorrections = true;

    // makes the navigation react to user inputs more predictively
    //   at varying frame rates
    bool fpsCompensation = true;

    bool debugRenderObjectPosition = false;
    bool debugRenderTargetPosition = false;
    bool debugRenderAltitudeSurrogates = false;
    bool debugRenderCameraObstructionSurrogates = false;
};

} // namespace vts

#endif
