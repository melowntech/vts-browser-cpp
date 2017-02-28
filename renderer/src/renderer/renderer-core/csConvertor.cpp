#include <vts-libs/vts/csconvertor.hpp>
#include <GeographicLib/Geodesic.hpp>
#include <ogr_spatialref.h>

#include "csConvertor.h"

namespace melown
{

class CsConvertorImpl : public CsConvertor
{
public:
    CsConvertorImpl(const std::string &phys,
                    const std::string &nav,
                    const std::string &pub,
                    const vtslibs::registry::Registry &registry) :
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
            auto r = registry.srs(nav).srsDef.reference();
            geodesic_.emplace(r.GetSemiMajor(), r.GetInvFlattening());
        }
    }

    const vec3 navToPhys(const vec3 &value) override
    {
        return vecFromUblas<vec3>(navToPhys_(vecToUblas<math::Point3d>(value)));
    }

    const vec3 physToNav(const vec3 &value) override
    {
        return vecFromUblas<vec3>(physToNav_(vecToUblas<math::Point3d>(value)));
    }

    const vec3 navToPub(const vec3 &value) override
    {
        return vecFromUblas<vec3>(navToPub_(vecToUblas<math::Point3d>(value)));
    }

    const vec3 pubToNav(const vec3 &value) override
    {
        return vecFromUblas<vec3>(pubToNav_(vecToUblas<math::Point3d>(value)));
    }

    const vec3 pubToPhys(const vec3 &value) override
    {
        return vecFromUblas<vec3>(pubToPhys_(vecToUblas<math::Point3d>(value)));
    }

    const vec3 physToPub(const vec3 &value) override
    {
        return vecFromUblas<vec3>(physToPub_(vecToUblas<math::Point3d>(value)));
    }

    const vec3 navGeodesicDirect(const vec3 &latLon,
                                 double azimuth, double distance) override
    {
        vec3 res;
        geodesic_->Direct(latLon(0), latLon(1),
                          azimuth, distance, res(0), res(1));
        res(2) = latLon(2);
        return res;
    }

    vtslibs::vts::CsConvertor navToPhys_;
    vtslibs::vts::CsConvertor physToNav_;
    vtslibs::vts::CsConvertor navToPub_;
    vtslibs::vts::CsConvertor pubToNav_;
    vtslibs::vts::CsConvertor pubToPhys_;
    vtslibs::vts::CsConvertor physToPub_;
    boost::optional<GeographicLib::Geodesic> geodesic_;
};

CsConvertor *CsConvertor::create(const std::string &phys,
                                 const std::string &nav,
                                 const std::string &pub,
                                 const vtslibs::registry::Registry &registry)
{
    return new CsConvertorImpl(phys, nav, pub, registry);
}

CsConvertor::~CsConvertor()
{}

} // namespace melown
