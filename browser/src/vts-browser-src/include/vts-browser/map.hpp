#ifndef MAP_H_jihsefk
#define MAP_H_jihsefk

#include <string>
#include <memory>
#include <vector>

#include "foundation.hpp"

namespace vts
{

enum class Srs
{
    Physical,
    Navigation,
    Public,
};

class VTS_API Map
{
public:
    Map(const class MapCreateOptions &options);
    virtual ~Map();
    
    void setMapConfigPath(const std::string &mapConfigPath,
                          const std::string &authPath = "");
    const std::string &getMapConfigPath() const;
    void purgeTraverseCache(bool hard);
    
    /// returns whether the map config has been downloaded
    /// and parsed successfully
    /// most other functions will not work until this is ready
    bool isMapConfigReady() const;
    /// returns whether the map has all resources needed for complete
    /// render
    bool isMapRenderComplete() const;
    /// returns estimation of progress till complete render
    double getMapRenderProgress() const;
    
    void dataInitialize(const std::shared_ptr<class Fetcher> &fetcher);
    bool dataTick();
    void dataFinalize();

    void renderInitialize();
    void renderTick(uint32 width, uint32 height);
    void renderFinalize();
    
    class MapCallbacks &callbacks();
    class MapStatistics &statistics();
    class MapOptions &options();
    class MapDraws &draws();
    class MapCredits &credits();

    void pan(const double value[3]);
    void pan(const double (&value)[3]);
    void rotate(const double value[3]);
    void rotate(const double (&value)[3]);
    void setPositionSubjective(bool subjective, bool convert);
    bool getPositionSubjective() const;
    void setPositionPoint(const double point[3]); /// navigation srs
    void setPositionPoint(const double (&point)[3]); /// navigation srs
    void getPositionPoint(double point[3]) const; /// navigation srs
    void setPositionRotation(const double point[3]); /// degrees
    void setPositionRotation(const double (&point)[3]); /// degrees
    void getPositionRotation(double point[3]) const; /// degrees
    void setPositionViewExtent(double viewExtent); /// physical length
    double getPositionViewExtent() const; /// physical length
    void setPositionFov(double fov); /// degrees
    double getPositionFov() const; /// degrees
    std::string getPositionJson() const;
    void setPositionJson(const std::string &position);
    void resetPositionAltitude();
    void resetPositionRotation(bool immediate);
    void resetNavigationMode();
    void setAutoMotion(const double value[3]);
    void setAutoMotion(const double (&value)[3]);
    void getAutoMotion(double value[3]) const;
    void setAutoRotation(const double value[3]);
    void setAutoRotation(const double (&value)[3]);
    void getAutoRotation(double value[3]) const;
    
    void convert(const double pointFrom[3], double pointTo[3],
                Srs srsFrom, Srs srsTo) const;
    
    const std::vector<std::string> getResourceSurfaces() const;
    const std::vector<std::string> getResourceBoundLayers() const;
    const std::vector<std::string> getResourceFreeLayers() const;
    
    std::vector<std::string> getViewNames() const;
    std::string getViewCurrent() const;
    void setViewCurrent(const std::string &name);
    void getViewData(const std::string &name, class MapView &view) const;
    void setViewData(const std::string &name, const class MapView &view);
    std::string getViewJson(const std::string &name) const;
    void setViewJson(const std::string &name, const std::string &view);
    
    void printDebugInfo();

private:
    std::shared_ptr<class MapImpl> impl;
};

} // namespace vts

#endif
