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

#ifndef COORDSMANIP_HPP_erxtz
#define COORDSMANIP_HPP_erxtz

#include <string>
#include <memory>

#include "include/vts-browser/math.hpp"

namespace vtslibs { namespace vts {
    struct MapConfig;
} }

namespace vts
{

class CoordManip : private Immovable
{
    // this class is just an interface
    //  - do not instantiate it directly - use the create method instead
    //  - do not inherit from it
public:
    static std::shared_ptr<CoordManip> create(
            vtslibs::vts::MapConfig &mapconfig,
            const std::string &searchSrs,
            const std::string &customSrs1,
            const std::string &customSrs2);

    vec3 navToPhys(const vec3 &value);
    vec3 physToNav(const vec3 &value);
    vec3 searchToNav(const vec3 &value);

    vec3 convert(const vec3 &value, Srs from, Srs to);
    vec3 convert(const vec3 &value, const std::string &from, Srs to);
    vec3 convert(const vec3 &value, Srs from, const std::string &to);

    vec3 geoDirect(const vec3 &position, double distance,
                              double azimuthIn, double &azimuthOut);
    vec3 geoDirect(const vec3 &position, double distance, double azimuthIn);
    void geoInverse(const vec3 &posA, const vec3 &posB,
                    double &distance, double &azimuthA, double &azimuthB);
    double geoAzimuth(const vec3 &a, const vec3 &b);
    double geoDistance(const vec3 &a, const vec3 &b);
    double geoArcDist(const vec3 &a, const vec3 &b);
};

} // namespace vts

#endif
