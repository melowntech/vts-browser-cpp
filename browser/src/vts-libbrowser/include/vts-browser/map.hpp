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
    Map(const class MapCreateOptions &options,
        const std::shared_ptr<class Fetcher> &fetcher);
    ~Map();

    // mapconfigPath: url to mapconfig
    // authPath: url to authentication server,
    //    alternatively, it may contain special value 'token:???'
    //    which is taken directly as authentication token instead of url
    void setMapconfigPath(const std::string &mapconfigPath,
                          const std::string &authPath = "");
    std::string getMapconfigPath() const;

    void purgeViewCache();
    void purgeDiskCache();

    // returns whether the mapconfig has been downloaded
    //   and parsed successfully
    // most other functions will not work until this returns true
    bool getMapconfigAvailable() const;

    // returns whether the mapconfig and all other required
    //   external definitions has been downloaded
    //   and parsed successfully
    // some other functions will not work until this returns true
    bool getMapconfigReady() const;

    bool getMapProjected() const;

    // returns whether the map has all resources needed for complete
    //   render
    bool getMapRenderComplete() const;

    // returns estimation of progress till complete render
    double getMapRenderProgress() const;

    void dataInitialize();
    // dataTick does at most MapOptions.maxResourceProcessesPerTick
    //   operations and returns
    // you should call it periodically
    void dataUpdate();
    void dataFinalize();

    // blocking alternative to:
    //   { dataInitialize();
    //     while (someCondition) dataTick();
    //     dataFinalize(); }
    // dataAllRun will return after renderFinalize has been called
    // the dataAllRun must be called on a separate thread,
    //   but is more cpu efficient than dataTick
    void dataAllRun();

    void renderInitialize();
    void renderUpdate(double elapsedTime); // seconds since last call
    void renderFinalize();

    // create new camera
    // you may have multiple cameras in single map
    std::shared_ptr<class Camera> camera();

    class MapCallbacks &callbacks();
    class MapStatistics &statistics();
    class MapRuntimeOptions &options();
    const class MapCelestialBody &celestialBody();
    std::shared_ptr<void> atmosphereDensityTexture();

    // srs conversion
    void convert(const double pointFrom[3], double pointTo[3],
                Srs srsFrom, Srs srsTo) const;
    void convert(const std::array<double, 3> &pointFrom, double pointTo[3],
                Srs srsFrom, Srs srsTo) const;

    // surfaces and layers resources
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

    // view manipulation
    std::vector<std::string> getViewNames() const;
    std::string getViewCurrent() const;
    std::string getViewJson(const std::string &name) const;
    class MapView getViewData(const std::string &name) const;
    void setViewCurrent(const std::string &name);
    void setViewJson(const std::string &name, const std::string &view);
    void setViewData(const std::string &name, const class MapView &view);
    void removeView(const std::string &name);

    // searching
    bool searchable() const;
    std::shared_ptr<class SearchTask> search(const std::string &query);
    std::shared_ptr<class SearchTask> search(const std::string &query,
                                const double point[3]); // navigation srs
    std::shared_ptr<class SearchTask> search(const std::string &query,
                     const std::array<double, 3> &lst); // navigation srs

private:
    std::shared_ptr<class MapImpl> impl;
};

} // namespace vts

#endif
