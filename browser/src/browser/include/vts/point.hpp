#ifndef POINT_H_eserbgf
#define POINT_H_eserbgf

#include "foundation.hpp"

namespace vts
{

struct VTS_API Point
{
    Point();
    Point(double x, double y, double z);
    
    union
    {
        struct
        {
            double data[3];
        };
        struct
        {
            double x, y, z;
        };
    };
};

} // namespace vts

#endif
