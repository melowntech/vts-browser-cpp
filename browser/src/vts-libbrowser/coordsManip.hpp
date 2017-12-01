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

#ifndef CSCONVERTER_H_erxtz
#define CSCONVERTER_H_erxtz

#include <string>
#include <memory>

#include <vts-libs/vts/mapconfig.hpp>

#include "include/vts-browser/math.hpp"

namespace vts
{

class CoordManip
{
public:
    static std::shared_ptr<CoordManip> create(
            vtslibs::vts::MapConfig &mapconfig,
            const std::string &searchSrs,
            const std::string &customSrs1,
            const std::string &customSrs2);

    virtual ~CoordManip();

    virtual vec3 navToPhys(const vec3 &value) = 0;
    virtual vec3 physToNav(const vec3 &value) = 0;
    virtual vec3 searchToNav(const vec3 &value) = 0;

    virtual vec3 convert(const vec3 &value, Srs from, Srs to) = 0;
    virtual vec3 convert(const vec3 &value,
            const vtslibs::registry::ReferenceFrame::Division::Node &from,
            Srs to) = 0;
    virtual vec3 convert(const vec3 &value, Srs from,
            const vtslibs::registry::ReferenceFrame::Division::Node &to) = 0;

    virtual vec3 geoDirect(const vec3 &position, double distance,
                              double azimuthIn, double &azimuthOut) = 0;
    virtual vec3 geoDirect(const vec3 &position, double distance,
                                    double azimuthIn) = 0;
    virtual void geoInverse(const vec3 &posA, const vec3 &posB,
                    double &distance, double &azimuthA, double &azimuthB) = 0;
    virtual double geoAzimuth(const vec3 &a, const vec3 &b) = 0;
    virtual double geoDistance(const vec3 &a, const vec3 &b) = 0;
    virtual double geoArcDist(const vec3 &a, const vec3 &b) = 0;
};

} // namespace vts

#endif
