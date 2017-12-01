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

#define Node vtslibs::registry::ReferenceFrame::Division::Node

namespace std
{
    template<> struct hash<vts::Srs>
    {
        size_t operator()(vts::Srs srs) const
        {
            return (int)srs;
        }
    };
}

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
    vtslibs::vts::MapConfig &mapconfig;
    const std::string &searchSrs;
    const std::string &customSrs1;
    const std::string &customSrs2;

    std::unordered_map<int, std::string>idxToProj;
    std::unordered_map<std::string, int> projToIdx;
    std::unordered_map<Srs, int> srsToIdx;
    std::unordered_map<const Node*, int> nodeToIdx;
    std::unordered_map<int, std::unordered_map<int,
                    std::shared_ptr<vtslibs::vts::CsConvertor>>> convertors;
    int nextIdx;

    boost::optional<GeographicLib::Geodesic> geodesic_;

public:
    CoordManipImpl(
            vtslibs::vts::MapConfig &mapconfig,
            const std::string &searchSrs,
            const std::string &customSrs1,
            const std::string &customSrs2) :
        mapconfig(mapconfig), searchSrs(searchSrs),
        customSrs1(customSrs1), customSrs2(customSrs2),
        nextIdx(0)
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
        s.srsDef = def;
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

    vec3 navToPhys(const vec3 &value) override
    {
        return convert(value, Srs::Navigation, Srs::Physical);
    }

    vec3 physToNav(const vec3 &value) override
    {
        return convert(value, Srs::Physical, Srs::Navigation);
    }

    vec3 searchToNav(const vec3 &value) override
    {
        return convert(value, Srs::Search, Srs::Navigation);
    }

    vec3 convert(const vec3 &value, int from, int to)
    {
        std::shared_ptr<vtslibs::vts::CsConvertor> &p = convertors[from][to];
        if (!p)
        {
            p = std::make_shared<vtslibs::vts::CsConvertor>(
                idxToProj[from], idxToProj[to], mapconfig);
        }
        return vecFromUblas<vec3>((*p)(vecFromUblas<math::Point3>(value)));
    }

    int idx(const std::string &proj)
    {
        auto it = projToIdx.find(proj);
        if (it != projToIdx.end())
            return it->second;
        int i = nextIdx++;
        idxToProj[i] = proj;
        return projToIdx[proj] = i;
    }

    int idx(const Node &node)
    {
        const Node *n = &node;
        auto it = nodeToIdx.find(n);
        if (it != nodeToIdx.end())
            return it->second;
        return nodeToIdx[n] = idx(n->srs);
    }

    int idx(Srs srs)
    {
        auto it = srsToIdx.find(srs);
        if (it != srsToIdx.end())
            return it->second;
        return srsToIdx[srs] = idx(srsToProj(srs));
    }

    vec3 convert(const vec3 &value, Srs from, Srs to) override
    {
        return convert(value, idx(from), idx(to));
    }

    vec3 convert(const vec3 &value, const Node &from, Srs to) override
    {
        return convert(value, idx(from), idx(to));
    }

    vec3 convert(const vec3 &value, Srs from, const Node &to) override
    {
        return convert(value, idx(from), idx(to));
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
};

} // namespace

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
