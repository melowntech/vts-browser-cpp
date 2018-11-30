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
    NavigationOptions(const std::string &json);
    void applyJson(const std::string &json);
    std::string toJson() const;

    // multiplier that is applied to mouse input for the respective actions
    double sensitivityPan;
    double sensitivityZoom;
    double sensitivityRotate;

    // innertia coefficients [0 - 1) for smoothing the navigation changes
    // inertia of zero makes all changes to apply immediately
    // while inertia of almost one makes the moves very smooth and slow
    double inertiaPan;
    double inertiaZoom;
    double inertiaRotate;

    // lower and upper limit for view-extent
    // expressed as multiplicative factor of planet major radius
    double viewExtentLimitScaleMin;
    double viewExtentLimitScaleMax;

    // view-extent thresholds at which the camera normalization takes effect
    // expressed as multiplicative factor of planet major radius
    double viewExtentThresholdScaleLow;
    double viewExtentThresholdScaleHigh;

    // camera tilt limits (eg. -90 - 0)
    double tiltLimitAngleLow;
    double tiltLimitAngleHigh;

    // multiplicative factor at which camera altitude will converge to terrain
    //   when panning or zooming
    // range 0 (off) to 1 (fast)
    double cameraAltitudeFadeOutFactor;

    // latitude threshold (0 - 90) used for azimuthal navigation
    double navigationLatitudeThreshold;

    // maximum view-extent multiplier allowed for PIHA, must be larger than 1
    double navigationPihaViewExtentMult;
    // maximum position change allowed for PIHA, must be positive
    double navigationPihaPositionChange;

    // number of virtual samples to fit the view-extent
    // it is used to determine lod index at which to retrieve
    //   the altitude used to correct camera position
    uint32 navigationSamplesPerViewExtent;

    NavigationType navigationType;
    NavigationMode navigationMode;

    // limits camera tilt and yaw
    // uses viewExtentThresholdScaleLow/High
    // applies tiltLimitAngleLow/High (and yaw limit)
    bool cameraNormalization;

    // vertically converges objective position towards ground
    bool cameraAltitudeChanges;

    bool debugRenderObjectPosition;
    bool debugRenderTargetPosition;
};

} // namespace vts

#endif
