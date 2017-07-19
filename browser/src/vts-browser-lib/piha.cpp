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
#include "include/vts-browser/options.hpp"
#include "piha.hpp"

namespace vts
{

namespace
{

double signum(double a)
{
    return a < 0 ? -1 : a > 0 ? 1 : 0;
}

double sumExtents(double v1, double v2, double mult)
{
    assert(v1 > 0);
    assert(v2 > 0);
    assert(mult > 1);
    double steps = std::log(v2 / v1) / std::log(mult);
    steps = std::abs(steps);
    return std::min(v1, v2) * (std::pow(mult, steps) - 1) / (mult - 1);
}

} // namespace

void navigationPiha(
        const MapOptions &inOptions,
        NavigationType inType,
        double inTimeDiff,
        double inFov,
        double inHorizontalDistance,
        double inVerticalChange,
        double inStartViewExtent,
        double inViewExtentChange,
        vec3 inStartRotation,
        vec3 inRotationChange,
        double &outViewExtent,
        double &outHorizontalMove,
        double &outVerticalMove,
        vec3 &outRotation)
{
    if (inType == NavigationType::Instant)
    {
        outHorizontalMove = inHorizontalDistance;
        outVerticalMove = inVerticalChange;
        outRotation = inStartRotation + inRotationChange;
        outViewExtent = inStartViewExtent + inViewExtentChange;
        return;
    }

    if (inType == NavigationType::Quick)
    {
        outHorizontalMove = inHorizontalDistance
                * (1 - inOptions.cameraInertiaPan);
        outVerticalMove = inVerticalChange
                * (1 - inOptions.cameraInertiaPan);
        outRotation = inStartRotation + inRotationChange
                * (1 - inOptions.cameraInertiaRotate);
        outViewExtent = inStartViewExtent + inViewExtentChange
                * (1 - inOptions.cameraInertiaZoom);
        return;
    }

    double distanceWhileExtentChanges = sumExtents(inStartViewExtent,
            inStartViewExtent + inViewExtentChange,
            inOptions.navigationMaxViewExtentMult)
            * inOptions.navigationMaxPositionChange;

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
    double dc = inStartViewExtent * inOptions.navigationMaxPositionChange;
    double d = std::min(dr / (dc * 60 + 1), 1.0); // smooth landing
    double vf = std::sin(a) * d; // view extent factor
    double hf = std::cos(a) * d; // horizontal factor

    // view extent and horizontal move
    outViewExtent = inStartViewExtent
            * std::pow(inOptions.navigationMaxViewExtentMult, vf);
    outViewExtent = std::max(outViewExtent,
                        inStartViewExtent - std::abs(inViewExtentChange));
    outHorizontalMove = std::min(outViewExtent
                    * inOptions.navigationMaxPositionChange
                    * std::max(hf, 0.0), inHorizontalDistance);

    // vertical move and camera rotation
    double ch = inHorizontalDistance > 1 ? outHorizontalMove
                                           / inHorizontalDistance : 1;
    outVerticalMove = inVerticalChange * std::min(ch,
                                    1 - inOptions.cameraInertiaPan);
    outRotation = inStartRotation + inRotationChange * std::min(ch,
                                    1 - inOptions.cameraInertiaRotate);

    // todo better camera rotation in fly-over type

    (void)inTimeDiff;
    (void)inFov;
}

} // namespace vts
