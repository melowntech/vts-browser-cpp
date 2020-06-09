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

#ifndef CELESTIAL_HPP_eruiehowwedw
#define CELESTIAL_HPP_eruiehowwedw

#include <string>
#include "foundation.hpp"

namespace vts
{

class VTS_API MapCelestialBody
{
public:
    std::string name;
    double majorRadius = 0; // meters
    double minorRadius = 0; // meters

    struct VTS_API Atmosphere
    {
        // color of the sky (RGBA 0..1) near the horizon
        float colorHorizon[4] = {};
        // color of the sky (RGBA 0..1) directly overhead
        float colorZenith[4] = {};

        // Coefficient controlling the steepness of the color
        //   gradient between zenith and horizon.
        // The two colors are weighted by the formula
        //   T ^ colorGradientExponent,
        //   where T is the transmittance obtained by
        //   integrating the atmosphere density
        double colorGradientExponent = 0.3;

        // Height of the atmosphere in meters.
        // As the density of the atmosphere decreases exponentially,
        //   the "boundary" of the atmosphere is defined as the altitude
        //   where the density drops to one millionth (1e-6)
        //   of zero altitude density.
        double thickness = 0;

        // the coefficient 1e-6 in the definition of atmosphere thickness
        double thicknessQuantile = 1e-6;

        // Distance at which objects and color features
        //   are no longer clearly discernible.
        // We define this to be the distance (at zero altitude)
        //   at which an object's color is attenuated to 1 %
        //   of its original value by atmospheric effects.
        double visibility = 0;

        // The coefficient 1e-2 in the definition of the visibility
        double visibilityQuantile = 1e-2;
    } atmosphere;
};

VTS_API void atmosphereDerivedAttributes(const MapCelestialBody &body,
    double &boundaryThickness,
    double &horizontalExponent, double &verticalExponent);

} // namespace vts

#endif
