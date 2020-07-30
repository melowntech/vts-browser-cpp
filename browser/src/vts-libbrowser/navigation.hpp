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

#include <vts-libs/registry/referenceframe.hpp>

#include "include/vts-browser/math.hpp"
#include "include/vts-browser/navigationOptions.hpp"

namespace vts
{

class CameraImpl;
class Navigation;
class TemporalNavigationState;

class NavigationImpl : private Immovable
{
public:
    typedef vtslibs::registry::Position::Type Type;
    typedef vtslibs::registry::Position::HeightMode HeightMode;

    CameraImpl *const camera = nullptr;
    Navigation *const navigation = nullptr;
    NavigationOptions options;
    vec3 position {0,0,0};
    vec3 targetPosition {0,0,0};
    vec3 orientation {0,0,0};
    vec3 targetOrientation {0,0,0};
    double verticalExtent = 0;
    double targetVerticalExtent = 0;
    double verticalFov = 0;
    double autoRotation = 0;
    boost::optional<double> lastPositionAltitude;
    boost::optional<double> positionAltitudeReset;
    std::shared_ptr<TemporalNavigationState> temporalNavigationState;
    std::vector<std::pair<double, double>> normalizationSmoothing; // elapsedTime, tilt
    Type type = Type::objective;
    HeightMode heightMode = HeightMode::fixed;
    NavigationMode mode = NavigationMode::Azimuthal;
    bool suspendAltitudeChange = false;

    NavigationImpl(CameraImpl *cam, Navigation *navigation);
    void initialize();
    void pan(vec3 value);
    void rotate(vec3 value);
    void zoom(double value);
    void resetNavigationMode();
    void convertSubjObj();
    double objectiveDistance();
    void positionToCamera(vec3 &center, vec3 &dir, vec3 &up, const vec3 &inputRotation, const vec3 &inputPosition);
    bool isNavigationModeValid() const;
    void setManual();
    void setPosition(const vtslibs::registry::Position &position); // set target position
    vtslibs::registry::Position getPosition() const; // return camera position
    void updateNavigation(double elapsedTime);
};

void normalizeOrientation(vec3 &o);

} // namespace vts

#endif
