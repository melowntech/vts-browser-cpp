#ifndef CSCONVERTER_H_erxtz
#define CSCONVERTER_H_erxtz

#include <string>
#include <memory>

#include "math.hpp"

namespace vtslibs { namespace registry {

class Registry;

} } // namespace vtslibs::registry

namespace vts
{

class CsConvertor
{
public:
    static CsConvertor *create(const std::string &phys,
                               const std::string &nav,
                               const std::string &pub,
                               const vtslibs::registry::Registry &registry);

    virtual ~CsConvertor();

    virtual const vec3 navToPhys(const vec3 &value) = 0;
    virtual const vec3 physToNav(const vec3 &value) = 0;
    virtual const vec3 navToPub (const vec3 &value) = 0;
    virtual const vec3 pubToNav (const vec3 &value) = 0;
    virtual const vec3 pubToPhys(const vec3 &value) = 0;
    virtual const vec3 physToPub(const vec3 &value) = 0;
    
    virtual const vec3 convert(const vec3 &value,
                               const std::string &from,
                               const std::string &to) = 0;

    virtual const vec3 navGeodesicDirect(const vec3 &position,
                                         double azimuth, double distance) = 0;
};

} // namespace vts

#endif
