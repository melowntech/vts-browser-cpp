#ifndef VIEW_H_wqfzugsahakwejgr
#define VIEW_H_wqfzugsahakwejgr

#include <string>
#include <map>

#include "foundation.hpp"

namespace vts
{

class VTS_API MapView
{
public:
    class BoundLayerInfo
    {
    public:
        typedef std::vector<BoundLayerInfo> Map;
        
        std::string id;
        double alpha;
        
        BoundLayerInfo();
        BoundLayerInfo(const std::string &id);
    };
    
    class SurfaceInfo
    {
    public:
        typedef std::map<std::string, SurfaceInfo> Map;
        
        BoundLayerInfo::Map boundLayers;
    };
    
    class FreeLayerInfo
    {
    public:
        typedef std::map<std::string, FreeLayerInfo> Map;
        
        std::string style;
        BoundLayerInfo::Map boundLayers;
    };
    
    std::string description;
    SurfaceInfo::Map surfaces;
    FreeLayerInfo::Map freeLayers;
};

} // namespace vts

#endif
