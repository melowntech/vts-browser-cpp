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

#include <ogr_spatialref.h>

#include "map.hpp"
#include "utilities/json.hpp"

namespace vts
{

MapCelestialBody::MapCelestialBody() :
    majorRadius(0), minorRadius(0)
{}

MapCelestialBody::Atmosphere::Atmosphere() :
    colorHorizon{0,0,0,0}, colorZenith{0,0,0,0},
    colorGradientExponent(0.3),
    thickness(0), thicknessQuantile(1e-6),
    visibility(0), visibilityQuantile(1e-2)
{}

bool MapConfig::isEarth() const
{
    auto n = srs(referenceFrame.model.physicalSrs);
    auto r = n.srsDef.reference();
    auto a = r.GetSemiMajor();
    return std::abs(a - 6378137) < 50000;
}

void MapConfig::initializeCelestialBody() const
{
    map->body = MapCelestialBody();
    {
        // find body radius based on reference frame
        auto n = srs(referenceFrame.model.physicalSrs);
        auto r = n.srsDef.reference();
        map->body.majorRadius = r.GetSemiMajor();
        map->body.minorRadius = r.GetSemiMinor();
    }

    MapCelestialBody::Atmosphere &a = map->body.atmosphere;

    // load body from mapconfig
    if (map->mapConfig->referenceFrame.body)
    {
        const auto &b = map->mapConfig->bodies.get(
            *map->mapConfig->referenceFrame.body);
        Json::Value j = boost::any_cast<Json::Value>(b.json);
        map->body.name = j["name"].asString();
        if (j.isMember("atmosphere"))
        {
            Json::Value &ja = j["atmosphere"];
#define J(NAME) if (ja.isMember(#NAME)) \
                    a.NAME = ja[#NAME].asDouble();
            J(thickness);
            J(thicknessQuantile);
            J(visibility);
            J(visibilityQuantile);
            J(colorGradientExponent);
#undef J
            for (uint32 i = 0; i < 4; i++)
            {
#define C(NAME) if (ja[#NAME].isArray() && i < ja[#NAME].size()) \
                    a.NAME[i] = ja[#NAME][i].asFloat();
                C(colorHorizon);
                C(colorZenith);
#undef C
            }
        }
    }

    // mapconfig does not define the body
    // try to detect it and use defaults
    else if (isEarth())
    {
        map->body.name = "Earth";
        a.thickness = 100000;
        a.visibility = 100000;
        a.colorHorizon = { 158, 206, 255, 255 };
        a.colorZenith = { 62, 120, 229, 255 };
    }
    else if (std::abs(map->body.majorRadius - 3396200) < 30000)
    {
        map->body.name = "Mars";
        a.thickness = 50000;
        a.visibility = 200000;
        a.colorHorizon = { 115, 100, 74, 255 };
        a.colorZenith = { 115, 100, 74, 255 };
    }
    else
    {
        map->body.name = "<unknown>";
    }

    // normalize colors
    for (int i = 0; i < 4; i++)
    {
        a.colorHorizon[i] /= 255;
        a.colorZenith[i] /= 255;
    }
}

void AtmosphereDerivedAttributes(const MapCelestialBody &body,
    double &boundaryThickness,
    double &horizontalExponent, double &verticalExponent)
{
    boundaryThickness = horizontalExponent = verticalExponent = 0;
    if (body.majorRadius <= 0 || body.atmosphere.thickness <= 0)
        return;
    const auto &a = body.atmosphere;
    // todo recompute the boundaryThickness with predefined quantile
    boundaryThickness = a.thickness;
    verticalExponent = std::log(1 / a.thicknessQuantile);
    horizontalExponent = body.majorRadius
        * std::log(1 / a.visibilityQuantile) / a.visibility;
}

} // namespace vts
