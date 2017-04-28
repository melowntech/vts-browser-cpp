#include <unordered_map>
#include <memory>

#include <vts-libs/vts/csconvertor.hpp>
#include <GeographicLib/Geodesic.hpp>
#include <ogr_spatialref.h>

#include "csConvertor.hpp"

// todo use void pj_set_finder( const char *(*)(const char *) );

namespace vts
{

namespace
{
    struct DummyDeleter
    {
        void operator()(vtslibs::vts::CsConvertor *)
        {}
    };
}

class CsConvertorImpl : public CsConvertor
{
public:
    CsConvertorImpl(const std::string &phys,
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
            geodesic_.emplace(r.GetSemiMajor(), r.GetInvFlattening());
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
    
    const vec3 convert(const vec3 &value,
                                    const std::string &from,
                                    const std::string &to) override
    {
        std::shared_ptr<vtslibs::vts::CsConvertor> &p = convertors[from][to];
        if (!p)
            p = std::shared_ptr<vtslibs::vts::CsConvertor>(
                        new vtslibs::vts::CsConvertor(from, to, registry));
        return vecFromUblas<vec3>((*p)(vecFromUblas<math::Point3>(value)));
    }

    const vec3 navGeodesicDirect(const vec3 &position,
                                 double azimuth, double distance) override
    {
        vec3 res;
        geodesic_->Direct(position(1), position(0),
                          azimuth, distance, res(1), res(0));
        res(2) = position(2);
        return res;
    }

    vtslibs::vts::CsConvertor navToPhys_;
    vtslibs::vts::CsConvertor physToNav_;
    vtslibs::vts::CsConvertor navToPub_;
    vtslibs::vts::CsConvertor pubToNav_;
    vtslibs::vts::CsConvertor pubToPhys_;
    vtslibs::vts::CsConvertor physToPub_;
    std::unordered_map<std::string, std::unordered_map<std::string,
            std::shared_ptr<vtslibs::vts::CsConvertor>>> convertors;
    boost::optional<GeographicLib::Geodesic> geodesic_;
    const vtslibs::registry::Registry &registry;
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

} // namespace vts
