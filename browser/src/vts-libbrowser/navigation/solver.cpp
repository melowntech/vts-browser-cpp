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

class TemporalNavigationState
{
public:
    double parabolaParameter;

    TemporalNavigationState() : parabolaParameter(nan1())
    {}
};

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

/*
double distanceByExtent(double startExtent, double finalExtent,
    double extentFactor, double distanceFactor, uint32 &depth)
{
    depth++;
    if (startExtent * extentFactor > finalExtent
        && startExtent / extentFactor < finalExtent)
        return 0;
    return startExtent * distanceFactor
        + distanceByExtent(startExtent > finalExtent
            ? startExtent / extentFactor
            : startExtent * extentFactor,
            finalExtent, extentFactor, distanceFactor, depth);
}

double ballisticAngle(double x, double y, double g)
{
    return std::atan(y / x + std::sqrt((y*y) / (x*x) + 1));
}

double ballisticVelocity(double x, double y, double g)
{
    return std::sqrt((y + std::sqrt((y*y) + (x+x))) * g);
}
*/


void parabola(
    double sx, double sy, // start at (sx,sy); finish at (0,0)
    double a, // parabola parameter
    double &dx, double &dy, double &outLen) // output
{
    // vertically flip the parabola
    a *= -1;

    // y = a*x*x + b*x + c
    // parabola goes through (0,0) -> c = 0

    // parabola goes through (sx,sy) -> b = (y - a*x*x) / x
    double b = (sy - a * sx*sx) / sx;
    // y = a*x^2 - a*x*sx + x*sy/sx

    // tangent y = k * (x - sx) + sy
    double k = a * sx + sy / sx;

    // output direction
    dx = -1 / std::sqrt(k*k + 1);
    dy = dx * k;

    // output is normalized
    assert(std::abs(std::sqrt(dx*dx + dy*dy) - 1) < 1e-5);

    // output length
    auto &ff = [a, b](double x)
    {
        double p = 2 * a * x + b;
        return (std::sqrt(p * p + 1) * p + std::asinh(p)) / (4 * a);
    };
    outLen = ff(sx) - ff(0);
}

// Perceptively Invariant Horizontal Autopilot
// (piha = freckle in czech)
void flyOver(
    const NavigationOptions &navOpts,
    std::shared_ptr<TemporalNavigationState> &temporalNavigationState,
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
    if (!temporalNavigationState)
    {
        temporalNavigationState
            = std::make_shared<TemporalNavigationState>();
        temporalNavigationState->parabolaParameter
            = 3.0 / (inHorizontalDistance + 1);
    }

    double distance = std::sqrt(inVerticalChange * inVerticalChange
        + inHorizontalDistance * inHorizontalDistance);
    double dx, dy, dl;
    parabola(distance, -inViewExtentChange,
        temporalNavigationState->parabolaParameter,
        dx, dy, dl);
    double spd = inStartViewExtent * 0.1 * timeDiff;
    double move = -dx * spd;
    move /= distance;
    outHorizontalMove = inHorizontalDistance * move;
    outVerticalMove = inVerticalChange * move;
    outViewExtent = inStartViewExtent + dy * spd;
    outRotation = inStartRotation;


    /*
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



    // extent or translation
    double exFacForSm = std::pow(
        navOpts.navigationPihaViewExtentChange + 1, 30);
    uint32 depthBefore = 0, depthAfter = 0;
    double distanceBefore = distanceByExtent(
        inStartViewExtent / exFacForSm,
        inStartViewExtent + inViewExtentChange,
        extentChangeFactor, positionChangeFactor, depthBefore);
    double distanceAfter = distanceByExtent(
        inStartViewExtent * exFacForSm,
        inStartViewExtent + inViewExtentChange,
        extentChangeFactor, positionChangeFactor, depthAfter);
    assert(distanceAfter > distanceBefore);
    double extentOrTranslation = (translationDist - distanceBefore)
        / (distanceAfter - distanceBefore);
    extentOrTranslation = 2 * clamp(extentOrTranslation, 0, 1) - 1;
    // extentOrTranslation:
    // -1 -> zoom in
    // 0 -> translation
    // +1 -> zoom out
    bool zoomIn = extentOrTranslation < 0;
    extentOrTranslation = std::abs(extentOrTranslation);
    extentChangeFactor = (extentChangeFactor - 1)
        * (extentOrTranslation) + 1;
    positionChangeFactor *= (1 - extentOrTranslation);

    if (zoomIn)
        extentChangeFactor = 1 / extentChangeFactor;

    outViewExtent = inStartViewExtent * extentChangeFactor;
    outHorizontalMove = inStartViewExtent * positionChangeFactor;
    outVerticalMove = inVerticalChange;
    outRotation = inStartRotation + inRotationChange;

    // rotation
    double rotationDist = length(inRotationChange);

    // steps (frames) to complete individual operations
    double extentSteps = extentChangeFactor < 1 + 1e-7 ? 0
        : std::abs(std::log((inStartViewExtent
        + inViewExtentChange) / inStartViewExtent))
        / std::log(extentChangeFactor);
    double translationSteps = positionChangeFactor < 1e-7 ? 0
        : translationDist / (inStartViewExtent * positionChangeFactor);
    double rotationSteps = rotationDist
        / (360 * rotationChangeFactor);
    assert(extentSteps >= 0);
    assert(translationSteps >= 0);
    assert(rotationSteps >= 0);

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
    if (zoomIn)
        extentFactor = 1.0 / extentFactor;
    outViewExtent = inStartViewExtent * extentFactor;
    outHorizontalMove = horizontalChange * totalStepsInv;
    outVerticalMove = verticalChange * totalStepsInv;
    outRotation = inStartRotation + inRotationChange * totalStepsInv;
    */
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
        std::shared_ptr<TemporalNavigationState> &temporalNavigationState,
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
    assert(timeDiff >= 0);
    assert(inHorizontalDistance >= 0);
    assert(inStartViewExtent > 0);
    assert(inStartViewExtent + inViewExtentChange > 0);

    if (timeDiff < 1e-7
        || (inHorizontalDistance < 1e-7
        && std::abs(inVerticalChange) < 1e-7
        && std::abs(inViewExtentChange) < 1e-7
        && vectorAbsSum(inRotationChange) < 1e-7))
    {
        // no operation needed
        outViewExtent = inStartViewExtent;
        outHorizontalMove = 0;
        outVerticalMove = 0;
        outRotation = inStartRotation;
        temporalNavigationState.reset();
    }
    else switch (navOpts.navigationType)
    {
    case NavigationType::Instant:
        outHorizontalMove = inHorizontalDistance;
        outVerticalMove = inVerticalChange;
        outRotation = inStartRotation + inRotationChange;
        outViewExtent = inStartViewExtent + inViewExtentChange;
        temporalNavigationState.reset();
        break;
    case NavigationType::Quick:
        outHorizontalMove = inHorizontalDistance
            * inertiaFactor(navOpts, timeDiff, navOpts.inertiaPan);
        outVerticalMove = inVerticalChange
            * inertiaFactor(navOpts, timeDiff, navOpts.inertiaPan);
        outRotation = inStartRotation + inRotationChange
            * inertiaFactor(navOpts, timeDiff, navOpts.inertiaRotate);
        outViewExtent = inStartViewExtent + inViewExtentChange
            * inertiaFactor(navOpts, timeDiff, navOpts.inertiaZoom);
        temporalNavigationState.reset();
        break;
    case NavigationType::FlyOver:
        flyOver(
            navOpts,
            temporalNavigationState,
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
        break;
    }

    assert(outViewExtent >= 0);
    assert(outHorizontalMove >= 0);
    assert(!std::isnan(outVerticalMove));

    (void)fov;
}

} // namespace vts
