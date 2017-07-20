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

#include <unordered_map>
#include <memory>

#include <boost/utility/in_place_factory.hpp>

#include <vts-libs/vts/csconvertor.hpp>
#include <GeographicLib/Geodesic.hpp>
#include <ogr_spatialref.h>

#include "coordsManip.hpp"

#include <gdalwarper.h>

// todo use void pj_set_finder( const char *(*)(const char *) );

namespace vts
{

namespace
{

void GDALErrorHandlerEmpty(CPLErr, int, const char *)
{
    // do nothing    
}

struct gdalInitClass
{
    gdalInitClass()
    {
        CPLSetErrorHandler(&GDALErrorHandlerEmpty);
    }
} gdalInitInstance;

class CoordManipImpl : public CoordManip
{
    static const char *searchKey;
    
public:
    CoordManipImpl(const std::string &phys,
                   const std::string &nav,
                   const std::string &pub,
                   const std::string &search,
                   vtslibs::registry::Registry &registry) :
        registry(registry), phys(phys), nav(nav), pub(pub)
    {
        auto n = registry.srs(nav);
        //if (n.type == vtslibs::registry::Srs::Type::geographic)
        {
            auto r = n.srsDef.reference();
            auto a = r.GetSemiMajor();
            auto b = r.GetSemiMinor();
            geodesic_ = boost::in_place(a, (a - b) / a);
        }
        {
            vtslibs::registry::Srs s;
            s.comment = "search";
            s.srsDef = search;
            registry.srs.replace(searchKey, s);
        }
    }
    
    vec3 navToPhys(const vec3 &value) override
    {
        return convert(value, nav, phys);
    }

    vec3 physToNav(const vec3 &value) override
    {
        return convert(value, phys, nav);
    }

    vec3 navToPub(const vec3 &value) override
    {
        return convert(value, nav, pub);
    }

    vec3 pubToNav(const vec3 &value) override
    {
        return convert(value, pub, nav);
    }

    vec3 pubToPhys(const vec3 &value) override
    {
        return convert(value, pub, phys);
    }

    vec3 physToPub(const vec3 &value) override
    {
        return convert(value, phys, pub);
    }

    vec3 searchToNav(const vec3 &value) override
    {
        return convert(value, searchKey, nav);
    }
    
    vec3 convert(const vec3 &value, const std::string &from,
                                    const std::string &to) override
    {
        std::shared_ptr<vtslibs::vts::CsConvertor> &p = convertors[from][to];
        if (!p)
            p = std::make_shared<vtslibs::vts::CsConvertor>(from, to, registry);
        return vecFromUblas<vec3>((*p)(vecFromUblas<math::Point3>(value)));
    }

    vec3 geoDirect(const vec3 &position, double distance,
                                 double azimuthIn, double &azimuthOut) override
    {
        vec3 res;
        geodesic_->Direct(position(1), position(0), azimuthIn,
                          distance, res(1), res(0), azimuthOut);
        res(2) = position(2);
        return res;
    }
    
    vec3 geoDirect(const vec3 &position, double distance,
                                 double azimuthIn) override
    {
        double a;
        return geoDirect(position, distance, azimuthIn, a);
    }
    
    void geoInverse(const vec3 &a, const vec3 &b,
            double &distance, double &azimuthA, double &azimuthB) override
    {
        geodesic_->Inverse(a(1), a(0), b(1), b(0),
                           distance, azimuthA, azimuthB);
    }
    
    double geoAzimuth(const vec3 &a, const vec3 &b) override
    {
        double d, a1, a2;
        geoInverse(a, b, d, a1, a2);
        return a1;
    }
    
    double geoDistance(const vec3 &a, const vec3 &b) override
    {
        double d, a1, a2;
        geoInverse(a, b, d, a1, a2);
        return d;
    }
    
    double geoArcDist(const vec3 &a, const vec3 &b) override
    {
        double dummy;
        return geodesic_->Inverse(a(1), a(0), b(1), b(0), dummy);
    }

    const vtslibs::registry::Registry &registry;
    const std::string &phys;
    const std::string &nav;
    const std::string &pub;
    std::unordered_map<std::string, std::unordered_map<std::string,
            std::shared_ptr<vtslibs::vts::CsConvertor>>> convertors;
    boost::optional<GeographicLib::Geodesic> geodesic_;
};

const char *CoordManipImpl::searchKey = "$search$";

} // namespace

std::shared_ptr<CoordManip> CoordManip::create(const std::string &phys,
                                 const std::string &nav,
                                 const std::string &pub,
                                 const std::string &search,
                                 vtslibs::registry::Registry &registry)
{
    return std::make_shared<CoordManipImpl>(phys, nav, pub, search, registry);
}

CoordManip::~CoordManip()
{}

} // namespace vts
