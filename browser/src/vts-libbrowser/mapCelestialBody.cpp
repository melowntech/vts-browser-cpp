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

#include "../map.hpp"

namespace vts
{

MapCelestialBody::MapCelestialBody() :
    majorRadius(0), minorRadius(0)
{}

MapCelestialBody::Atmosphere::Atmosphere() : thickness(0),
    horizontalExponent(0), verticalExponent(0),
    colorLow{0,0,0,0}, colorHigh{0,0,0,0}
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
    auto n = srs(referenceFrame.model.physicalSrs);
    auto r = n.srsDef.reference();
    map->body.majorRadius = r.GetSemiMajor();
    map->body.minorRadius = r.GetSemiMinor();
    MapCelestialBody::Atmosphere &a = map->body.atmosphere;
    if (isEarth())
    {
        map->body.name = "Earth";
        a.thickness = map->body.majorRadius * 0.01;
        a.horizontalExponent = 420;
        a.verticalExponent = 7;
        static vec4 lowColor = vec4(158, 206, 255, 255) / 255;
        static vec4 highColor = vec4(62, 120, 229, 255) / 255;
        for (int i = 0; i < 4; i++)
        {
            a.colorLow[i] = lowColor[i];
            a.colorHigh[i] = highColor[i];
        }
    }
    else if (std::abs(map->body.majorRadius - 3396200) < 30000)
    {
        map->body.name = "Mars";
        a.thickness = map->body.majorRadius * 0.01;
        a.horizontalExponent = 180;
        a.verticalExponent = 7;
        static vec4 lowColor = vec4(115, 100, 74, 255) / 255;
        static vec4 highColor = vec4(115, 100, 74, 255) / 255;
        for (int i = 0; i < 4; i++)
        {
            a.colorLow[i] = lowColor[i];
            a.colorHigh[i] = highColor[i];
        }
    }
    else
    {
        map->body.name = "<unknown>";
    }
}

} // namespace vts
