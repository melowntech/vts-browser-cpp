#ifndef CSCONVERTER_H_erxtz
#define CSCONVERTER_H_erxtz

#include <string>
#include <memory>

#include "math.h"

namespace vadstena
{
    namespace registry
    {
        class Registry;
    }
}

namespace melown
{
    class CsConvertorImpl;

    class CsConvertor
    {
    public:
        void configure(const std::string &phys, const std::string &nav, const std::string &pub, const vadstena::registry::Registry &registry);
        bool configured() const;

        const vec3 navToPhys(const vec3 &value);
        const vec3 physToNav(const vec3 &value);
        const vec3 navToPub (const vec3 &value);
        const vec3 pubToNav (const vec3 &value);
        const vec3 pubToPhys(const vec3 &value);
        const vec3 physToPub(const vec3 &value);

        const vec3 navGeodesicDirect(const vec3 &latLon, double azimuth, double distance);

    private:
        std::shared_ptr<CsConvertorImpl> impl;
    };
}

#endif
