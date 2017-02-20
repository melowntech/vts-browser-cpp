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

        const vec3 navToPhys(const vec3 &value);
        const vec3 physToNav(const vec3 &value);
        const vec3 navToPub (const vec3 &value);
        const vec3 pubToNav (const vec3 &value);
        const vec3 pubToPhys(const vec3 &value);
        const vec3 physToPub(const vec3 &value);

        /*
        const math::Point3 navToPhys(const math::Point3 &value);
        const math::Point3 physToNav(const math::Point3 &value);
        const math::Point3 navToPub (const math::Point3 &value);
        const math::Point3 pubToNav (const math::Point3 &value);
        const math::Point3 pubToPhys(const math::Point3 &value);
        const math::Point3 physToPub(const math::Point3 &value);
        */

    private:
        std::shared_ptr<CsConvertorImpl> impl;
    };
}

#endif
