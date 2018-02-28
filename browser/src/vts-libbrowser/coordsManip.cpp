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
#include <functional>

#include <boost/utility/in_place_factory.hpp>

#include <vts-libs/vts/csconvertor.hpp>
#include <GeographicLib/Geodesic.hpp>
#include <ogr_spatialref.h>

#include "coordsManip.hpp"

#include <gdalwarper.h>
#include <proj_api.h>

#define Node vtslibs::registry::ReferenceFrame::Division::Node

namespace vts
{

std::function<const char *(const char *)> projFinder;

namespace
{

const char *pjFind(const char *p)
{
    if (projFinder)
        return projFinder(p);
    return nullptr;
}

void GDALErrorHandlerEmpty(CPLErr, int, const char *)
{
    // do nothing
}

struct gdalInitClass
{
    gdalInitClass()
    {
        pj_set_finder(&pjFind);
        CPLSetErrorHandler(&GDALErrorHandlerEmpty);
    }
} gdalInitInstance;

class CoordManipImpl : public CoordManip
{
    vtslibs::vts::MapConfig &mapconfig;

    std::unordered_map<std::string, std::shared_ptr<vtslibs::vts::CsConvertor>>
        convertors;

    boost::optional<GeographicLib::Geodesic> geodesic_;

public:
    CoordManipImpl(
            vtslibs::vts::MapConfig &mapconfig,
            const std::string &searchSrs,
            const std::string &customSrs1,
            const std::string &customSrs2) :
        mapconfig(mapconfig)
    {
        // create geodesic
        {
            auto r = mapconfig.srs(mapconfig.referenceFrame.model.navigationSrs)
                        .srsDef.reference();
            auto a = r.GetSemiMajor();
            auto b = r.GetSemiMinor();
            geodesic_ = boost::in_place(a, (a - b) / a);
        }
        addSrsDef("$search$", searchSrs);
        addSrsDef("$custom1$", customSrs1);
        addSrsDef("$custom2$", customSrs2);
    }

    void addSrsDef(const std::string &name, const std::string &def)
    {
        vtslibs::registry::Srs s;
        s.comment = name;
        s.srsDef = geo::SrsDefinition::fromString(def);
        mapconfig.srs.replace(name, s);
    }

    std::string srsToProj(Srs srs)
    {
        switch(srs)
        {
        case Srs::Physical:
            return mapconfig.referenceFrame.model.physicalSrs;
        case Srs::Navigation:
            return mapconfig.referenceFrame.model.navigationSrs;
        case Srs::Public:
            return mapconfig.referenceFrame.model.publicSrs;
        case Srs::Search:
            return "$search$";
        case Srs::Custom1:
            return "$custom1$";
        case Srs::Custom2:
            return "$custom2$";
        default:
            LOGTHROW(fatal, std::invalid_argument) << "Invalid srs enum";
            throw;
        }
    }

    vtslibs::vts::CsConvertor &convertor(const std::string &a,
                                         const std::string &b)
    {
        std::string key = a + " >>> " + b;
        auto it = convertors.find(key);
        if (it == convertors.end())
            return *(convertors[key]
                    = std::make_shared<vtslibs::vts::CsConvertor>(
                        a, b, mapconfig));
        return *it->second;
    }

    vec3 convert(const vec3 &value, Srs from, Srs to) override
    {
        auto cs = convertor(srsToProj(from), srsToProj(to));
        return vecFromUblas<vec3>(cs(vecFromUblas<math::Point3>(value)));
    }

    vec3 convert(const vec3 &value, const Node &from, Srs to) override
    {
        auto cs = convertor(from.srs, srsToProj(to));
        return vecFromUblas<vec3>(cs(vecFromUblas<math::Point3>(value)));
    }

    vec3 convert(const vec3 &value, Srs from, const Node &to) override
    {
        auto cs = convertor(srsToProj(from), to.srs);
        return vecFromUblas<vec3>(cs(vecFromUblas<math::Point3>(value)));
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

    void geoInverse(const vec3 &a, const vec3 &b,
            double &distance, double &azimuthA, double &azimuthB) override
    {
        geodesic_->Inverse(a(1), a(0), b(1), b(0),
                           distance, azimuthA, azimuthB);
    }

    double geoArcDist(const vec3 &a, const vec3 &b) override
    {
        double dummy;
        return geodesic_->Inverse(a(1), a(0), b(1), b(0), dummy);
    }
};

} // namespace

vec3 CoordManip::navToPhys(const vec3 &value)
{
    return convert(value, Srs::Navigation, Srs::Physical);
}

vec3 CoordManip::physToNav(const vec3 &value)
{
    return convert(value, Srs::Physical, Srs::Navigation);
}

vec3 CoordManip::searchToNav(const vec3 &value)
{
    return convert(value, Srs::Search, Srs::Navigation);
}

vec3 CoordManip::geoDirect(const vec3 &position, double distance,
                             double azimuthIn)
{
    double a;
    return geoDirect(position, distance, azimuthIn, a);
}

double CoordManip::geoAzimuth(const vec3 &a, const vec3 &b)
{
    double d, a1, a2;
    geoInverse(a, b, d, a1, a2);
    return a1;
}

double CoordManip::geoDistance(const vec3 &a, const vec3 &b)
{
    double d, a1, a2;
    geoInverse(a, b, d, a1, a2);
    return d;
}

std::shared_ptr<CoordManip> CoordManip::create(
        vtslibs::vts::MapConfig &mapconfig,
        const std::string &searchSrs,
        const std::string &customSrs1,
        const std::string &customSrs2)
{
    return std::make_shared<CoordManipImpl>(
                mapconfig, searchSrs, customSrs1, customSrs2);
}

CoordManip::~CoordManip()
{}

} // namespace vts
