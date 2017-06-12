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

#include "include/vts-browser/math.hpp"

namespace vtslibs { namespace registry {

class Registry;

} } // namespace vtslibs::registry

namespace vts
{

class CoordManip
{
public:
    static std::shared_ptr<CoordManip> create(const std::string &phys,
                               const std::string &nav,
                               const std::string &pub,
                               const vtslibs::registry::Registry &registry);

    virtual ~CoordManip();

    virtual vec3 navToPhys(const vec3 &value) = 0;
    virtual vec3 physToNav(const vec3 &value) = 0;
    virtual vec3 navToPub (const vec3 &value) = 0;
    virtual vec3 pubToNav (const vec3 &value) = 0;
    virtual vec3 pubToPhys(const vec3 &value) = 0;
    virtual vec3 physToPub(const vec3 &value) = 0;

    virtual vec3 convert(const vec3 &value,
                         const std::string &from,
                         const std::string &to) = 0;

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
