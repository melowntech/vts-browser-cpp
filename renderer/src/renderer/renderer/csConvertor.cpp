#include "../../vts-libs/vts/csconvertor.hpp"

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
        {}

        vadstena::vts::CsConvertor navToPhys;
        vadstena::vts::CsConvertor physToNav;
        vadstena::vts::CsConvertor navToPub;
        vadstena::vts::CsConvertor pubToNav;
        vadstena::vts::CsConvertor pubToPhys;
        vadstena::vts::CsConvertor physToPub;
    };

    void CsConvertor::configure(const std::string &phys, const std::string &nav, const std::string &pub, const vadstena::registry::Registry &registry)
    {
        impl = std::shared_ptr<CsConvertorImpl>(new CsConvertorImpl(phys, nav, pub, registry));
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

    /*
    const math::Point3 CsConvertor::navToPhys(const math::Point3 &value)
    {
        return impl->navToPhys(value);
    }

    const math::Point3 CsConvertor::physToNav(const math::Point3 &value)
    {
        return impl->physToNav(value);
    }

    const math::Point3 CsConvertor::navToPub(const math::Point3 &value)
    {
        return impl->navToPub(value);
    }

    const math::Point3 CsConvertor::pubToNav(const math::Point3 &value)
    {
        return impl->pubToNav(value);
    }

    const math::Point3 CsConvertor::pubToPhys(const math::Point3 &value)
    {
        return impl->pubToPhys(value);
    }

    const math::Point3 CsConvertor::physToPub(const math::Point3 &value)
    {
        return impl->physToPub(value);
    }
    */
}
