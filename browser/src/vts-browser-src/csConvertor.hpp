#ifndef CSCONVERTER_H_erxtz
#define CSCONVERTER_H_erxtz

#include <string>
#include <memory>

#include "include/vts-browser/math.hpp"

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

    virtual vec3 navToPhys(const vec3 &value) = 0;
    virtual vec3 physToNav(const vec3 &value) = 0;
    virtual vec3 navToPub (const vec3 &value) = 0;
    virtual vec3 pubToNav (const vec3 &value) = 0;
    virtual vec3 pubToPhys(const vec3 &value) = 0;
    virtual vec3 physToPub(const vec3 &value) = 0;

    virtual vec3 convert(const vec3 &value,
                         const std::string &from,
                         const std::string &to) = 0;

    virtual vec3 navGeodesicDirect(const vec3 &position, double distance,
                              double azimuthIn, double &azimuthOut) = 0;
    virtual vec3 navGeodesicDirect(const vec3 &position, double distance,
                                    double azimuthIn) = 0;
    virtual double navGeodesicDistance(const vec3 &a, const vec3 &b) = 0;
};

} // namespace vts

#endif
