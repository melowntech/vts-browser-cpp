#ifndef CREDITS_H_wefherjkgf
#define CREDITS_H_wefherjkgf

#include <string>
#include <vector>

#include "foundation.hpp"

namespace vts
{

class VTS_API MapCredits
{
public:
    struct Credit
    {
        std::string notice;
        std::string url;
        uint32 hits;
        uint32 maxLod;
    };
    
    struct Scope
    {
        std::vector<Credit> credits;
    };
    
    Scope imagery;
    Scope data;
    
    const std::string textShort() const;
    const std::string textFull() const;
};

} // namespace vts

#endif
