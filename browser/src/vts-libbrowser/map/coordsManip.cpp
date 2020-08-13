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

#include "../coordsManip.hpp"

#include "../include/vts-browser/mapCallbacks.hpp" // ensure that projFinderCallback is visible

#include <boost/utility/in_place_factory.hpp>
#include <GeographicLib/Geodesic.hpp>
#include "geo/detail/projapi.hpp"

#include <vts-libs/vts/csconvertor.hpp>
#include <vts-libs/vts/mapconfig.hpp>

#include <unordered_map>
#include <memory>
#include <functional>

namespace vts
{

std::function<const char *(const char *)> &projFinderCallback()
{
    static std::function<const char *(const char *)> fnc;
    return fnc;
}

namespace
{

const char *pjFind(const char *p)
{
    if (!p)
        return nullptr; // nothing to look for? nothing to return.
    if (projFinderCallback())
        return projFinderCallback()(p);
    return nullptr;
}

// disable PROJ's file access on some systems
PAFile pjFOpen(projCtx ctx, const char *filename, const char *access)
{
    return nullptr;
}
size_t pjFRead(void *buffer, size_t size, size_t nmemb, PAFile file)
{
    return 0;
}
int pjFSeek(PAFile file, long offset, int whence)
{
    return 0;
}
long pjFTell(PAFile file)
{
    return 0;
}
void pjFClose(PAFile file)
{}

struct projInitClass
{
    projFileAPI_t pjFileApi;

    projInitClass()
    {
        pjFileApi.FOpen = &pjFOpen;
        pjFileApi.FRead = &pjFRead;
        pjFileApi.FSeek = &pjFSeek;
        pjFileApi.FTell = &pjFTell;
        pjFileApi.FClose = &pjFClose;
        pj_set_finder(&pjFind);
    }
} projInitInstance;

class CoordManipImpl : public CoordManip
{
public:
    vtslibs::vts::MapConfig &mapconfig;
    std::unordered_map<std::string, std::unique_ptr<vtslibs::vts::CsConvertor>> convertors;
    boost::optional<GeographicLib::Geodesic> geodesic_;
    projCtx ctx = nullptr;

    CoordManipImpl(
            vtslibs::vts::MapConfig &mapconfig,
            const std::string &searchSrs,
            const std::string &customSrs1,
            const std::string &customSrs2) :
        mapconfig(mapconfig),
        ctx(pj_ctx_alloc())
    {
        LOG(info1) << "Creating coordinate systems manipulator";

#ifdef __EMSCRIPTEN__
        pj_ctx_set_fileapi(ctx, &projInitInstance.pjFileApi);
#endif

        // create geodesic
        {
            auto r = geo::ellipsoid(mapconfig.srs(mapconfig.referenceFrame.model.navigationSrs).srsDef);
            auto a = r[0];
            auto b = r[2];
            LOG(info1) << "Creating geodesic manipulator: a=<" << a << ">, b=<" << b << ">";
            geodesic_ = boost::in_place(a, (a - b) / a);
        }

        addSrsDef("$search$", searchSrs);
        addSrsDef("$custom1$", customSrs1);
        addSrsDef("$custom2$", customSrs2);
    }

    ~CoordManipImpl()
    {
        pj_ctx_free(ctx);
    }

    void addSrsDef(const std::string &name, const std::string &def)
    {
        vtslibs::registry::Srs s;
        s.comment = name;
        s.srsDef = geo::SrsDefinition::fromString(def);
        mapconfig.srs.replace(name, s);
    }

    const std::string &srsToProj(Srs srs)
    {
        static const std::string search = "$search$";
        static const std::string custom1 = "$custom1$";
        static const std::string custom2 = "$custom2$";
        switch(srs)
        {
        case Srs::Physical:
            return mapconfig.referenceFrame.model.physicalSrs;
        case Srs::Navigation:
            return mapconfig.referenceFrame.model.navigationSrs;
        case Srs::Public:
            return mapconfig.referenceFrame.model.publicSrs;
        case Srs::Search:
            return search;
        case Srs::Custom1:
            return custom1;
        case Srs::Custom2:
            return custom2;
        default:
            LOGTHROW(fatal, std::invalid_argument) << "Invalid srs enum";
            throw;
        }
    }

    vtslibs::vts::CsConvertor &convertor(const std::string &a, const std::string &b)
    {
        const std::string key = a + " >>> " + b;
        auto it = convertors.find(key);
        if (it == convertors.end())
        {
            convertors[key] = std::make_unique<vtslibs::vts::CsConvertor>(a, b, mapconfig, ctx);
            it = convertors.find(key);
        }
        return *it->second;
    }

    vec3 convert(const vec3 &value, const std::string &f, const std::string &t)
    {
        const auto &cs = convertor(f, t);
        vec3 res = vecFromUblas<vec3>(cs(vecFromUblas<math::Point3>(value)));
        //LOG(debug) << "Converted <" << value.transpose() << "><" << f << "> to <" << res.transpose() << "><" << t << ">";
        return res;
    }
};

} // namespace

std::shared_ptr<CoordManip> CoordManip::create(
    vtslibs::vts::MapConfig &mapconfig,
    const std::string &searchSrs,
    const std::string &customSrs1,
    const std::string &customSrs2)
{
    return std::make_shared<CoordManipImpl>(mapconfig, searchSrs, customSrs1, customSrs2);
}

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

vec3 CoordManip::convert(const vec3 &value, Srs from, Srs to)
{
    CoordManipImpl *impl = (CoordManipImpl *)this;
    return impl->convert(value, impl->srsToProj(from), impl->srsToProj(to));
}

vec3 CoordManip::convert(const vec3 &value, const std::string &from, Srs to)
{
    CoordManipImpl *impl = (CoordManipImpl *)this;
    return impl->convert(value, from, impl->srsToProj(to));
}

vec3 CoordManip::convert(const vec3 &value, Srs from, const std::string &to)
{
    CoordManipImpl *impl = (CoordManipImpl *)this;
    return impl->convert(value, impl->srsToProj(from), to);
}

vec3 CoordManip::geoDirect(const vec3 &position, double distance, double azimuthIn, double &azimuthOut)
{
    CoordManipImpl *impl = (CoordManipImpl *)this;
    vec3 res;
    impl->geodesic_->Direct(position(1), position(0), azimuthIn, distance, res(1), res(0), azimuthOut);
    res(2) = position(2);
    return res;
}

vec3 CoordManip::geoDirect(const vec3 &position, double distance, double azimuthIn)
{
    double a;
    return geoDirect(position, distance, azimuthIn, a);
}

void CoordManip::geoInverse(const vec3 &a, const vec3 &b, double &distance, double &azimuthA, double &azimuthB)
{
    CoordManipImpl *impl = (CoordManipImpl *)this;
    impl->geodesic_->Inverse(a(1), a(0), b(1), b(0), distance, azimuthA, azimuthB);
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

double CoordManip::geoArcDist(const vec3 &a, const vec3 &b)
{
    CoordManipImpl *impl = (CoordManipImpl *)this;
    double dummy;
    return impl->geodesic_->Inverse(a(1), a(0), b(1), b(0), dummy);
}

} // namespace vts
