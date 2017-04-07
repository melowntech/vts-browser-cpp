#ifndef MAP_H_jihsefk
#define MAP_H_jihsefk

#include <string>
#include <memory>
#include <functional>

#include "foundation.hpp"

namespace vts
{

struct VTS_API Point
{
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

enum class Srs
{
    Physical,
    Navigation,
    Public,
};

class VTS_API MapFoundation
{
public:
    MapFoundation();
    virtual ~MapFoundation();
    
    std::function<std::shared_ptr<class GpuTexture>(const std::string &)>
            createTexture;
    std::function<std::shared_ptr<class GpuMesh>(const std::string &)>
            createMesh;
    
    void dataInitialize(class Fetcher *fetcher);
    bool dataTick();
    void dataFinalize();

    void renderInitialize();
    void renderTick(uint32 width, uint32 height);
    void renderFinalize();

    void setMapConfig(const std::string &mapConfigPath);
    void pan(const double value[3]);
    void rotate(const double value[3]);

    class MapStatistics &statistics();
    class MapOptions &options();
    class DrawBatch &drawBatch();
    
    void setPositionPoint(const Point &point); /// navigation srs
    const Point getPositionPoint(); /// navigation srs
    void setPositionRotation(const Point &point); /// degrees
    const Point getPositionRotation(); /// degrees
    void setPositionViewExtent(double viewExtent); /// physical length
    double getPositionViewExtent(); /// physical length
    void setPositionFov(double fov); /// degrees
    double getPositionFov(); /// degrees
    
    const Point convert(const Point &point, Srs from, Srs to);

private:
    std::shared_ptr<class MapImpl> impl;
};

} // namespace vts

#endif
