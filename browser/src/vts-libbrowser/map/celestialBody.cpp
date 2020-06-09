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

#include "../include/vts-browser/celestial.hpp"
#include "../utilities/json.hpp"
#include "../mapConfig.hpp"
#include "../map.hpp"

#include <vts-libs/vts/atmospheredensitytexture.hpp>
#include <vts-libs/vts/urltemplate.hpp>

namespace vts
{

using UrlTemplate = vtslibs::vts::UrlTemplate;

bool Mapconfig::isEarth() const
{
    double a = geo::ellipsoid(srs(referenceFrame.model.physicalSrs).srsDef)[0];
    return std::abs(a - 6378137) < 50000;
}

void Mapconfig::initializeCelestialBody()
{
    map->body = MapCelestialBody();
    {
        // find body radius based on reference frame
        auto r = geo::ellipsoid(srs(referenceFrame.model.physicalSrs).srsDef);
        map->body.majorRadius = r[0];
        map->body.minorRadius = r[2];
    }

    MapCelestialBody::Atmosphere &a = map->body.atmosphere;

    // load body from mapconfig
    if (referenceFrame.body)
    {
        const auto &b = bodies.get(*referenceFrame.body);
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
        vecToRaw(vec4f(158, 206, 255, 255), a.colorHorizon);
        vecToRaw(vec4f(62, 120, 229, 255), a.colorZenith);
    }
    else if (std::abs(map->body.majorRadius - 3396200) < 30000)
    {
        map->body.name = "Mars";
        a.thickness = 50000;
        a.visibility = 200000;
        vecToRaw(vec4f(115, 100, 74, 255), a.colorHorizon);
        vecToRaw(vec4f(115, 100, 74, 255), a.colorZenith);
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

    // atmosphere density texture
    if (navigationSrsType() != vtslibs::registry::Srs::Type::projected
        && map->body.majorRadius > 0
        && a.thickness > 0)
    {
        double a, b, c;
        atmosphereDerivedAttributes(map->body, a, b, c);
        vtslibs::vts::AtmosphereTextureSpec spec;
        spec.thickness = a / map->body.majorRadius;
        spec.verticalCoefficient = c;
        std::string name = spec.toQueryArg();
        if (map->mapconfig->services.has("atmdensity"))
        {
            UrlTemplate temp(map->mapconfig->services.get("atmdensity").url);
            UrlTemplate::Vars vars;
            vars.params = { name };
            name = temp(vars);
            name = convertPath(name, map->mapconfigPath);
        }
        else
            name = "generate://atmdensity-" + name;
        atmosphereDensityTextureName = name;
    }
}

void atmosphereDerivedAttributes(const MapCelestialBody &body,
    double &boundaryThickness,
    double &horizontalExponent, double &verticalExponent)
{
    boundaryThickness = horizontalExponent = verticalExponent = 0;
    if (body.majorRadius <= 0 || body.atmosphere.thickness <= 0)
        return;

    const auto &a = body.atmosphere;

    // using the atmosphere thickness quantile directly might lead to
    //   sharp edge at the atmosphere boundary,
    //   therefore we have to recompute the properties for a custom quantile
    static const double targetQuantile = 1e-6;
    double k = std::log(1 / a.thicknessQuantile) / a.thickness;
    boundaryThickness = std::log(1 / targetQuantile) / k;

    verticalExponent = std::log(1 / targetQuantile);

    horizontalExponent = body.majorRadius
        * std::log(1 / a.visibilityQuantile) / a.visibility * 5;
    // times 5 as compensation for texture normalization factor
}

} // namespace vts
