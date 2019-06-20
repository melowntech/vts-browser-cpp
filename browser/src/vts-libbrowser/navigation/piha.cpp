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

#include <dbglog/dbglog.hpp>
#include "../include/vts-browser/navigationOptions.hpp"
#include "piha.hpp"

namespace vts
{

namespace
{

double sumExtents(double v1, double v2, double mult)
{
    assert(v1 > 0);
    assert(v2 > 0);
    assert(mult > 1);
    double steps = std::log(v2 / v1) / std::log(mult);
    steps = std::abs(steps);
    return std::min(v1, v2) * (std::pow(mult, steps) - 1) / (mult - 1);
}

double inertiaFactor(
    const NavigationOptions &navOpts,
    double timeDiff,
    double inertia)
{
    if (!navOpts.fpsCompensation)
        return 1 - inertia;

    // the options inertia values are for 60 frames per second
    static const double nominalTime = 1.0 / 60;

    return 1 - std::pow(inertia, timeDiff / nominalTime);
}

} // namespace

void navigationPiha(
        const NavigationOptions &navOpts,
        double timeDiff,
        double fov,
        double inHorizontalDistance,
        double inVerticalChange,
        double inStartViewExtent,
        double inViewExtentChange,
        const vec3 &inStartRotation,
        const vec3 &inRotationChange,
        double &outViewExtent,
        double &outHorizontalMove,
        double &outVerticalMove,
        vec3 &outRotation)
{
    if (navOpts.navigationType == NavigationType::Instant)
    {
        outHorizontalMove = inHorizontalDistance;
        outVerticalMove = inVerticalChange;
        outRotation = inStartRotation + inRotationChange;
        outViewExtent = inStartViewExtent + inViewExtentChange;
        return;
    }

    if (navOpts.navigationType == NavigationType::Quick)
    {
        outHorizontalMove = inHorizontalDistance
                * inertiaFactor(navOpts, timeDiff, navOpts.inertiaPan);
        outVerticalMove = inVerticalChange
                * inertiaFactor(navOpts, timeDiff, navOpts.inertiaPan);
        outRotation = inStartRotation + inRotationChange
                * inertiaFactor(navOpts, timeDiff, navOpts.inertiaRotate);
        outViewExtent = inStartViewExtent + inViewExtentChange
                * inertiaFactor(navOpts, timeDiff, navOpts.inertiaZoom);
        return;
    }

    double distanceWhileExtentChanges = sumExtents(inStartViewExtent,
            inStartViewExtent + inViewExtentChange,
            navOpts.navigationPihaViewExtentMult)
            * navOpts.navigationPihaPositionChange;

    static const double piHalf = 3.14159265358979323846264338327 * 0.5;

    // ratio of horizontal movement to view extent change
    double a = std::numeric_limits<double>::quiet_NaN();
    if (inViewExtentChange > 0)
        a = piHalf;
    else
    {
        a = atan2(inHorizontalDistance, distanceWhileExtentChanges * 1.5);
        a = piHalf - (piHalf - a) * 2.0;
    }
    double dr = distanceWhileExtentChanges + inHorizontalDistance;
    double dc = inStartViewExtent * navOpts.navigationPihaPositionChange;
    double d = std::min(dr / (dc * 60 + 1), 1.0); // smooth landing
    double vf = std::sin(a) * d; // view extent factor
    double hf = std::cos(a) * d; // horizontal factor

    // view extent and horizontal move
    outViewExtent = inStartViewExtent
            * std::pow(navOpts.navigationPihaViewExtentMult, vf);
    outViewExtent = std::max(outViewExtent,
                        inStartViewExtent - std::abs(inViewExtentChange));
    outHorizontalMove = std::min(outViewExtent
                    * navOpts.navigationPihaPositionChange
                    * std::max(hf, 0.0), inHorizontalDistance);

    // vertical move and camera rotation
    double ch = inHorizontalDistance > 1
        ? outHorizontalMove / inHorizontalDistance : 1;
    outVerticalMove = inVerticalChange * std::min(ch,
        inertiaFactor(navOpts, timeDiff, navOpts.inertiaPan));
    outRotation = inStartRotation + inRotationChange * std::min(ch,
        inertiaFactor(navOpts, timeDiff, navOpts.inertiaRotate));

    // todo better camera rotation in fly-over type

    (void)timeDiff;
    (void)fov;
}

} // namespace vts
