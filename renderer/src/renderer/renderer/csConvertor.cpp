#include "../../vts-libs/vts/csconvertor.hpp"
#include <GeographicLib/Geodesic.hpp>
#include <ogr_spatialref.h>

#include "csConvertor.h"

namespace melown
{
    class CsConvertorImpl
    {
    public:
        CsConvertorImpl(const std::string &phys, const std::string &nav, const std::string &pub, const vadstena::registry::Registry &registry) :
            navToPhys(nav, phys, registry),
            physToNav(phys, nav, registry),
            navToPub (nav, pub , registry),
            pubToNav (pub, nav , registry),
            pubToPhys(pub, phys, registry),
            physToPub(phys, pub, registry)
        {
            auto n = registry.srs(nav);
            if (n.type == vadstena::registry::Srs::Type::geographic)
            {
                auto r = registry.srs(nav).srsDef.reference();
                geodesic.emplace(r.GetSemiMajor(), r.GetInvFlattening());
            }
        }

        vadstena::vts::CsConvertor navToPhys;
        vadstena::vts::CsConvertor physToNav;
        vadstena::vts::CsConvertor navToPub;
        vadstena::vts::CsConvertor pubToNav;
        vadstena::vts::CsConvertor pubToPhys;
        vadstena::vts::CsConvertor physToPub;
        boost::optional<GeographicLib::Geodesic> geodesic;
    };

    void CsConvertor::configure(const std::string &phys, const std::string &nav, const std::string &pub, const vadstena::registry::Registry &registry)
    {
        impl = std::shared_ptr<CsConvertorImpl>(new CsConvertorImpl(phys, nav, pub, registry));
    }

    bool CsConvertor::configured() const
    {
        return !!impl;
    }

    const vec3 CsConvertor::navToPhys(const vec3 &value)
    {
        return vecFromUblas<vec3>(impl->navToPhys(vecToUblas<math::Point3d>(value)));
    }

    const vec3 CsConvertor::physToNav(const vec3 &value)
    {
        return vecFromUblas<vec3>(impl->physToNav(vecToUblas<math::Point3d>(value)));
    }

    const vec3 CsConvertor::navToPub(const vec3 &value)
    {
        return vecFromUblas<vec3>(impl->navToPub(vecToUblas<math::Point3d>(value)));
    }

    const vec3 CsConvertor::pubToNav(const vec3 &value)
    {
        return vecFromUblas<vec3>(impl->pubToNav(vecToUblas<math::Point3d>(value)));
    }

    const vec3 CsConvertor::pubToPhys(const vec3 &value)
    {
        return vecFromUblas<vec3>(impl->pubToPhys(vecToUblas<math::Point3d>(value)));
    }

    const vec3 CsConvertor::physToPub(const vec3 &value)
    {
        return vecFromUblas<vec3>(impl->physToPub(vecToUblas<math::Point3d>(value)));
    }

    const vec3 CsConvertor::navGeodesicDirect(const vec3 &latLon, double azimuth, double distance)
    {
        vec3 res;
        impl->geodesic->Direct(latLon(0), latLon(1), azimuth, distance, res(0), res(1));
        res(2) = latLon(2);
        return res;
    }
}
