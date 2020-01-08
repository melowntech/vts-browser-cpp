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

#include "../include/vts-browser/navigationOptions.hpp"
#include "solver.hpp"

#include <dbglog/dbglog.hpp>

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

double sqr(double a)
{
    return a * a;
}

double hypot(double a, double b)
{
    // naive but faster than std::hypot
    return std::sqrt(sqr(a) + sqr(b));
}

double vectorAbsSum(const vec3 &v)
{
    return std::abs(v[0]) + std::abs(v[1]) + std::abs(v[2]);
}

void parabola(
    double sx, double sy, // start at (sx,sy); finish at (0,0)
    double a, // parabola parameter
    double &dx, double &dy, double &outLen) // output
{
    if (sx < 1e-7)
    {
        // just a vertical line
        dx = 0;
        dy = sy < 0 ? 1 : -1;
        outLen = std::abs(sy);
    }
    else
    {
        // generic parabola: y = a*x*x + b*x + c

        // vertically flip the parabola
        a *= -1;

        // our parabola goes through (0,0) -> c = 0
        // parabola goes through (sx,sy) -> b = (y - a*x*x) / x
        double b = (sy - a * sqr(sx)) / sx;
        // y = a*x^2 - a*x*sx + x*sy/sx

        // tangent y = k * (x - sx) + sy
        double k = a * sx + sy / sx;

        // output direction
        dx = -1 / std::sqrt(sqr(k) + 1);
        dy = dx * k;

        // output length
        const auto &ff = [a, b](double x)
        {
            double p = 2 * a * x + b;
            return (std::sqrt(sqr(p) + 1) * p + std::asinh(p)) / (4 * a);
        };
        outLen = ff(sx) - ff(0);
    }

    // output is normalized
    assert(std::abs(hypot(dx, dy) - 1) < 1e-5);
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
        // start of the fly over
        temporalNavigationState
            = std::make_shared<TemporalNavigationState>();
        temporalNavigationState->parabolaParameter
            = navOpts.flyOverSpikinessFactor
            / (inHorizontalDistance + 1);
    }

    // movement / view extent parameters
    double distance = hypot(inVerticalChange, inHorizontalDistance);
    double dx, dy, dl;
    parabola(distance, -inViewExtentChange,
        temporalNavigationState->parabolaParameter,
        dx, dy, dl);
    double moveSpeed = inStartViewExtent * timeDiff
        * navOpts.flyOverMotionChangeFraction;

    // normalization for multiple simultaneous movements
    double moveSteps = dl / moveSpeed;
    double rotationDist = length(inRotationChange);
    double rotationSteps = rotationDist
        / (90 * navOpts.flyOverRotationChangeSpeed * timeDiff);
    // add some additional steps to simulate slowdown at the end of the fly over
    double totalSteps = hypot(rotationSteps, moveSteps) + 10;

    // compute outputs
    moveSpeed = dl / totalSteps;
    assert(dx >= -1 && dx <= 0);
    double move = -dx * moveSpeed;
    move /= distance;
    outHorizontalMove = inHorizontalDistance * move;
    outVerticalMove = inVerticalChange * move;
    outViewExtent = inStartViewExtent + dy * moveSpeed;
    outRotation = inStartRotation + inRotationChange / totalSteps;
}

double inertiaFactor(
    const NavigationOptions &navOpts,
    double timeDiff,
    double inertia)
{
    if (!navOpts.fpsCompensation)
        return 1 - inertia;

    // the options values are for 60 frames per second
    static const double nominalFps = 60;

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
        double inViewExtentCurrent,
        double inViewExtentChange,
        const vec3 &inRotationCurrent,
        const vec3 &inRotationChange,
        double &outViewExtent,
        double &outHorizontalMove,
        double &outVerticalMove,
        vec3 &outRotation)
{
    assert(timeDiff >= 0);
    assert(inHorizontalDistance >= 0);
    assert(inViewExtentCurrent > 0);
    assert(inViewExtentCurrent + inViewExtentChange > 0);

    if (timeDiff < 1e-7
        || (inHorizontalDistance < 1e-7
        && std::abs(inVerticalChange) < 1e-7
        && std::abs(inViewExtentChange) < 1e-7
        && vectorAbsSum(inRotationChange) < 1e-7))
    {
        outHorizontalMove = 0;
        outVerticalMove = 0;
        outRotation = inRotationCurrent;
        outViewExtent = inViewExtentCurrent;
        temporalNavigationState.reset();
    }
    else switch (navOpts.type)
    {
    case NavigationType::Instant:
        outHorizontalMove = inHorizontalDistance;
        outVerticalMove = inVerticalChange;
        outRotation = inRotationCurrent + inRotationChange;
        outViewExtent = inViewExtentCurrent + inViewExtentChange;
        temporalNavigationState.reset();
        break;
    case NavigationType::Quick:
        outHorizontalMove = inHorizontalDistance
            * inertiaFactor(navOpts, timeDiff, navOpts.inertiaPan);
        outVerticalMove = inVerticalChange
            * inertiaFactor(navOpts, timeDiff, navOpts.inertiaPan);
        outRotation = inRotationCurrent + inRotationChange
            * inertiaFactor(navOpts, timeDiff, navOpts.inertiaRotate);
        outViewExtent = inViewExtentCurrent + inViewExtentChange
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
            inViewExtentCurrent,
            inViewExtentChange,
            inRotationCurrent,
            inRotationChange,
            outViewExtent,
            outHorizontalMove,
            outVerticalMove,
            outRotation
        );
        break;
    }

    assert(outViewExtent >= 0);
    assert(inViewExtentCurrent + outViewExtent > 0);
    assert(outHorizontalMove >= 0);
    assert(!std::isnan(outVerticalMove));

    (void)fov;
}

} // namespace vts
