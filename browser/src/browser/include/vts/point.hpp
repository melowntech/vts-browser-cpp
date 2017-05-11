#ifndef POINT_H_eserbgf
#define POINT_H_eserbgf

#include "foundation.hpp"

namespace vts
{

struct VTS_API Point
{
    Point();
    Point(double x, double y, double z);
    
    double &x() { return data[0]; }
    double &y() { return data[1]; }
    double &z() { return data[2]; }
    
    double data[3];
};

} // namespace vts

#endif
