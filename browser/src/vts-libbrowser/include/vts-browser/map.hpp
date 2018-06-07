/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MAP_HPP_jihsefk
#define MAP_HPP_jihsefk

#include <string>
#include <memory>
#include <vector>
#include <array>

#include "foundation.hpp"

namespace vts
{

// fundamental class which orchestrates all the vts tasks
class VTS_API Map
{
public:
    Map(const class MapCreateOptions &options, const std::shared_ptr<class Fetcher> &fetcher);
    virtual ~Map();

    // mapConfigPath: url to map config
    // authPath: url to authentication server,
    //    alternatively, it may contain special value 'token:???'
    //    which is taken directly as authentication token instead of url
    // sriPath: url for fast summary resource information retrieval
    void setMapConfigPath(const std::string &mapConfigPath,
                          const std::string &authPath = "",
                          const std::string &sriPath = "");
    std::string getMapConfigPath() const;

    void purgeViewCache();
    void purgeDiskCache();

    // returns whether the map config has been downloaded
    //   and parsed successfully
    // most other functions will not work until this returns true
    bool getMapConfigAvailable() const;
    // returns whether the map config and all other required
    //   external definitions has been downloaded
    //   and parsed successfully
    // some other functions will not work until this returns true
    bool getMapConfigReady() const;
    // returns whether the map has all resources needed for complete
    //   render
    bool getMapRenderComplete() const;
    // returns estimation of progress till complete render
    double getMapRenderProgress() const;

    void dataInitialize();
    // does at most MapOptions.maxResourceProcessesPerTick operations and returns
    // you should call it periodically
    void dataTick();
    // blocking alternative to dataTick
    // it will only return after renderFinalize has been called in the main thread
    // the dataRun must be called on a separate thread, but is more efficient
    void dataRun();
    void dataFinalize();

    void renderInitialize();
    void renderTickPrepare(double elapsedTime); // seconds since last call
    void renderTickRender();
    void renderFinalize();
    void setWindowSize(uint32 width, uint32 height);

    class MapCallbacks &callbacks();
    class MapStatistics &statistics();
    class MapOptions &options();
    class MapDraws &draws();
    const class MapCredits &credits();
    const class MapCelestialBody &celestialBody();

    void pan(const double value[3]);
    void pan(const std::array<double, 3> &lst);
    void rotate(const double value[3]);
    void rotate(const std::array<double, 3> &lst);
    void zoom(double value);

    // corrects current position altitude to match the surface
    void resetPositionAltitude();

    // this will reset navigation to azimuthal mode (or whichever is set)
    void resetNavigationMode();

    void setPositionSubjective(bool subjective, bool convert);
    void setPositionPoint(const double point[3]);
    void setPositionPoint(const std::array<double, 3> &lst);
    void setPositionRotation(const double rot[3]);
    void setPositionRotation(const std::array<double, 3> &lst);
    void setPositionViewExtent(double viewExtent);
    void setPositionFov(double fov);
    void setPositionJson(const std::string &position);
    void setPositionUrl(const std::string &position);
    void setAutoRotation(double value);

    bool getPositionSubjective() const;
    void getPositionPoint(double point[3]) const;
    void getPositionRotation(double point[3]) const;
    void getPositionRotationLimited(double point[3]) const;
    double getPositionViewExtent() const;
    double getPositionFov() const;
    std::string getPositionJson() const;
    std::string getPositionUrl() const;
    double getAutoRotation() const;

    void convert(const double pointFrom[3], double pointTo[3],
                Srs srsFrom, Srs srsTo) const;
    void convert(const std::array<double, 3> &pointFrom, double pointTo[3],
                Srs srsFrom, Srs srsTo) const;

    std::vector<std::string> getResourceSurfaces() const;
    std::vector<std::string> getResourceBoundLayers() const;
    std::vector<std::string> getResourceFreeLayers() const;
    FreeLayerType getResourceFreeLayerType(const std::string &name) const;

    // monolithic geodata free layer accessors
    void fabricateResourceFreeLayerGeodata(const std::string &name);
    std::string getResourceFreeLayerGeodata(const std::string &name) const;
    void setResourceFreeLayerGeodata(const std::string &name,
                                     const std::string &value);

    // any geodata free layer accessors
    std::string getResourceFreeLayerStyle(const std::string &name) const;
    void setResourceFreeLayerStyle(const std::string &name,
                                   const std::string &value);

    std::vector<std::string> getViewNames() const;
    std::string getViewCurrent() const;
    std::string getViewJson(const std::string &name) const;
    class MapView getViewData(const std::string &name) const;
    void setViewCurrent(const std::string &name);
    void setViewJson(const std::string &name, const std::string &view);
    void setViewData(const std::string &name, const class MapView &view);
    void removeView(const std::string &name);

    bool searchable() const;
    std::shared_ptr<class SearchTask> search(const std::string &query);
    std::shared_ptr<class SearchTask> search(const std::string &query,
                                const double point[3]); // navigation srs
    std::shared_ptr<class SearchTask> search(const std::string &query,
                     const std::array<double, 3> &lst); // navigation srs

    void printDebugInfo();

private:
    std::shared_ptr<class MapImpl> impl;
};

} // namespace vts

#endif
