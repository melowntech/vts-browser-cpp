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

struct DummyDeleter
{
    void operator()(vtslibs::vts::CsConvertor *)
    {}
};

class CoordManipImpl : public CoordManip
{
public:
    CoordManipImpl(const std::string &phys,
                    const std::string &nav,
                    const std::string &pub,
                    const vtslibs::registry::Registry &registry) :
        registry(registry),
        navToPhys_(nav, phys, registry),
        physToNav_(phys, nav, registry),
        navToPub_ (nav, pub , registry),
        pubToNav_ (pub, nav , registry),
        pubToPhys_(pub, phys, registry),
        physToPub_(phys, pub, registry)
    {
        auto n = registry.srs(nav);
        if (n.type == vtslibs::registry::Srs::Type::geographic)
        {
            auto r = n.srsDef.reference();
            geodesic_ = boost::in_place
                (r.GetSemiMajor(), 1 / r.GetInvFlattening());
        }
        convertors[nav][phys] = std::shared_ptr<vtslibs::vts::CsConvertor>(
                    &navToPhys_, DummyDeleter());
        convertors[phys][nav] = std::shared_ptr<vtslibs::vts::CsConvertor>(
                    &physToNav_, DummyDeleter());
        convertors[nav][pub] = std::shared_ptr<vtslibs::vts::CsConvertor>(
                    &navToPub_, DummyDeleter());
        convertors[pub][nav] = std::shared_ptr<vtslibs::vts::CsConvertor>(
                    &pubToNav_, DummyDeleter());
        convertors[pub][phys] = std::shared_ptr<vtslibs::vts::CsConvertor>(
                    &pubToPhys_, DummyDeleter());
        convertors[phys][pub] = std::shared_ptr<vtslibs::vts::CsConvertor>(
                    &physToPub_, DummyDeleter());
    }
    
    vec3 navToPhys(const vec3 &value) override
    {
        return vecFromUblas<vec3>(navToPhys_(vecToUblas<math::Point3d>(value)));
    }

    vec3 physToNav(const vec3 &value) override
    {
        return vecFromUblas<vec3>(physToNav_(vecToUblas<math::Point3d>(value)));
    }

    vec3 navToPub(const vec3 &value) override
    {
        return vecFromUblas<vec3>(navToPub_(vecToUblas<math::Point3d>(value)));
    }

    vec3 pubToNav(const vec3 &value) override
    {
        return vecFromUblas<vec3>(pubToNav_(vecToUblas<math::Point3d>(value)));
    }

    vec3 pubToPhys(const vec3 &value) override
    {
        return vecFromUblas<vec3>(pubToPhys_(vecToUblas<math::Point3d>(value)));
    }

    vec3 physToPub(const vec3 &value) override
    {
        return vecFromUblas<vec3>(physToPub_(vecToUblas<math::Point3d>(value)));
    }
    
    vec3 convert(const vec3 &value, const std::string &from,
                                    const std::string &to) override
    {
        std::shared_ptr<vtslibs::vts::CsConvertor> &p = convertors[from][to];
        if (!p)
            p = std::shared_ptr<vtslibs::vts::CsConvertor>(
                        new vtslibs::vts::CsConvertor(from, to, registry));
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
    vtslibs::vts::CsConvertor navToPhys_;
    vtslibs::vts::CsConvertor physToNav_;
    vtslibs::vts::CsConvertor navToPub_;
    vtslibs::vts::CsConvertor pubToNav_;
    vtslibs::vts::CsConvertor pubToPhys_;
    vtslibs::vts::CsConvertor physToPub_;
    std::unordered_map<std::string, std::unordered_map<std::string,
            std::shared_ptr<vtslibs::vts::CsConvertor>>> convertors;
    boost::optional<GeographicLib::Geodesic> geodesic_;
};

} // namespace

std::shared_ptr<CoordManip> CoordManip::create(const std::string &phys,
                                 const std::string &nav,
                                 const std::string &pub,
                                 const vtslibs::registry::Registry &registry)
{
    return std::make_shared<CoordManipImpl>(phys, nav, pub, registry);
}

CoordManip::~CoordManip()
{}

} // namespace vts
