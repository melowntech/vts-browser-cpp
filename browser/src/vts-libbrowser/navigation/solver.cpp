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
#include "solver.hpp"

namespace vts
{

namespace
{

// the options values are for 60 frames per second
const double nominalFps = 60;

double addFactor(
    const NavigationOptions &navOpts,
    double timeDiff,
    double factor)
{
    if (!navOpts.fpsCompensation)
        return factor;
    return factor * timeDiff * nominalFps;
}

double multFactor(
    const NavigationOptions &navOpts,
    double timeDiff,
    double factor)
{
    if (!navOpts.fpsCompensation)
        return factor;
    return std::pow(factor, timeDiff * nominalFps);
}

double sign(double a)
{
    if (a < 0)
        return -1;
    if (a > 0)
        return 1;
    return 0;
}

double clamp2(double v, double a, double b)
{
    return clamp(v, std::min(a, b), std::max(a, b));
}

double vectorAbsSum(const vec3 &v)
{
    return std::abs(v[0]) + std::abs(v[1]) + std::abs(v[2]);
}

// perceptively invariant horizontal autopilot
// piha = freckle in czech
void piha(
    const NavigationOptions &navOpts,
    double timeDiff,
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
    assert(timeDiff >= 0);
    assert(inHorizontalDistance >= 0);
    assert(inStartViewExtent > 0);
    assert(inStartViewExtent + inViewExtentChange > 0);

    // no operation needed
    if (inHorizontalDistance < 1e-7
        && std::abs(inVerticalChange) < 1e-7
        && std::abs(inViewExtentChange) < 1e-7
        && vectorAbsSum(inRotationChange) < 1e-7)
    {
        outViewExtent = inStartViewExtent;
        outHorizontalMove = 0;
        outVerticalMove = 0;
        outRotation = inStartRotation;
        return;
    }

    // fps compensation
    double extentChangeFactor = multFactor(navOpts, timeDiff,
        navOpts.navigationPihaViewExtentChange + 1);
    double positionChangeFactor = addFactor(navOpts, timeDiff,
        navOpts.navigationPihaPositionChange);
    double rotationChangeFactor = addFactor(navOpts, timeDiff,
        navOpts.navigationPihaRotationChange);
    assert(extentChangeFactor > 1);
    assert(positionChangeFactor > 0);
    assert(rotationChangeFactor > 0);

    // translations (as a combination of horizontal and vertical movement)
    double translationSum = inHorizontalDistance
        + std::abs(inVerticalChange);
    double translationDist = length(vec2(inHorizontalDistance,
        inVerticalChange));
    double translationFactor = (translationSum + 1)
        / (translationDist + 1);
    assert(translationFactor >= 1);
    double horizontalChange = inHorizontalDistance * translationFactor;
    double verticalChange = inVerticalChange * translationFactor;
    translationDist = length(vec2(horizontalChange, verticalChange));

    // rotation
    double rotationDist = length(inRotationChange);

    // steps (frames) to complete individual operations
    double extentSteps = std::abs(std::log((inStartViewExtent
        + inViewExtentChange) / inStartViewExtent))
        / std::log(extentChangeFactor);
    double translationSteps = translationDist
        / (inStartViewExtent * positionChangeFactor);
    double rotationSteps = rotationDist
        / (360 * rotationChangeFactor);
    assert(extentSteps >= 0);
    assert(translationSteps >= 0);
    assert(rotationSteps >= 0);

    // zoom-out to speed up translation
    if (translationSteps > 1e-7)
    {
        static const double piHalf = 3.14159265358979323846264338327 * 0.5;
        static const double piQuarter = 3.14159265358979323846264338327 * 0.25;
        double a = std::atan2(translationSteps, extentSteps);
        assert(a >= 0 && a <= piHalf);
        double b = piQuarter - std::abs(a - piQuarter);
        assert(b >= 0 && b <= piQuarter);
        b = std::pow(b / piQuarter, 0.5) * piQuarter;
        assert(b >= 0 && b <= piQuarter);
        double d = length(vec2(translationSteps, extentSteps));
        double t = std::sin(b) * d;
        double z = std::cos(b) * d;
        assert(t >= 0);
        assert(z >= 0);
        //assert(std::abs(t + z - translationSteps - extentSteps) < 1e-5);

        // update translation
        double tf = t / translationSteps;
        horizontalChange *= tf;
        verticalChange *= tf;
        translationSteps = t;

        // update view extent
        double e = std::exp(z * std::log(extentChangeFactor))
            * inStartViewExtent - inStartViewExtent;
        if (a < piQuarter && inViewExtentChange < 0)
            e *= -1;
        inViewExtentChange = e;
        extentSteps = z;
    }

    // inter-operation normalization
    double normalizationSum = extentSteps + translationSteps + rotationSteps;
    double normalizationDist = length(vec3(extentSteps,
        translationSteps, rotationSteps));
    double normalizationFactor = (normalizationSum + 1)
        / (normalizationDist + 1);
    assert(normalizationFactor >= 1);
    double totalSteps = std::max(extentSteps,
        std::max(translationSteps, rotationSteps))
        * normalizationFactor + 20;
    double totalStepsInv = 1.0 / totalSteps;
    assert(totalStepsInv >= 0 && totalStepsInv <= 1);

    // find the final outputs
    double extentFactor = std::pow(extentChangeFactor,
        extentSteps * totalStepsInv);
    if (inViewExtentChange < 0)
        extentFactor = 1.0 / extentFactor;
    outViewExtent = inStartViewExtent * extentFactor;
    outHorizontalMove = horizontalChange * totalStepsInv;
    outVerticalMove = verticalChange * totalStepsInv;
    outRotation = inStartRotation + inRotationChange * totalStepsInv;
}

double inertiaFactor(
    const NavigationOptions &navOpts,
    double timeDiff,
    double inertia)
{
    if (!navOpts.fpsCompensation)
        return 1 - inertia;
    return 1 - std::pow(inertia, timeDiff * nominalFps);
}

} // namespace

void solveNavigation(
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
    switch (navOpts.navigationType)
    {
    case NavigationType::Instant:
        outHorizontalMove = inHorizontalDistance;
        outVerticalMove = inVerticalChange;
        outRotation = inStartRotation + inRotationChange;
        outViewExtent = inStartViewExtent + inViewExtentChange;
        return;
    case NavigationType::Quick:
        outHorizontalMove = inHorizontalDistance
            * inertiaFactor(navOpts, timeDiff, navOpts.inertiaPan);
        outVerticalMove = inVerticalChange
            * inertiaFactor(navOpts, timeDiff, navOpts.inertiaPan);
        outRotation = inStartRotation + inRotationChange
            * inertiaFactor(navOpts, timeDiff, navOpts.inertiaRotate);
        outViewExtent = inStartViewExtent + inViewExtentChange
            * inertiaFactor(navOpts, timeDiff, navOpts.inertiaZoom);
        return;
    case NavigationType::FlyOver:
        return piha(
            navOpts,
            timeDiff,
            inHorizontalDistance,
            inVerticalChange,
            inStartViewExtent,
            inViewExtentChange,
            inStartRotation,
            inRotationChange,
            outViewExtent,
            outHorizontalMove,
            outVerticalMove,
            outRotation
        );
    }

    (void)fov;
}

} // namespace vts
