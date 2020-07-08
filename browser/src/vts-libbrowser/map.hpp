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

#ifndef MAP_HPP_cvukikljqwdf
#define MAP_HPP_cvukikljqwdf

#include <vector>

#include <vts-libs/registry/referenceframe.hpp>

#include "include/vts-browser/mapStatistics.hpp"
#include "include/vts-browser/mapOptions.hpp"
#include "include/vts-browser/mapCallbacks.hpp"
#include "include/vts-browser/celestial.hpp"
#include "include/vts-browser/math.hpp"
#include "include/vts-browser/buffer.hpp"

#include "validity.hpp"

namespace vts
{

class Map;
class Resources;
class Cache;
class Fetcher;
class AuthConfig;
class Mapconfig;
class CoordManip;
class Credits;
class MapLayer;
class CameraImpl;
class SearchTask;
class TraverseNode;

class Resource;
class GpuTexture;
class GpuAtmosphereDensityTexture;
class GpuMesh;
class MetaTile;
class MeshAggregate;
class ExternalBoundLayer;
class ExternalFreeLayer;
class BoundMetaTile;
class SearchTaskImpl;
class TilesetMapping;
class GeodataFeatures;
class GeodataStylesheet;
class GeodataStylesheet;
class GeodataTile;
class GpuFont;

using TileId = vtslibs::registry::ReferenceFrame::Division::Node::Id;

class MapImpl : private Immovable
{
public:
    Map *const map = nullptr;
    const MapCreateOptions createOptions;
    MapCallbacks callbacks;
    MapStatistics statistics;
    MapRuntimeOptions options;
    MapCelestialBody body;
    std::shared_ptr<Resources> resources;
    std::shared_ptr<Cache> cache;
    std::shared_ptr<Fetcher> fetcher;
    std::shared_ptr<AuthConfig> auth;
    std::shared_ptr<Mapconfig> mapconfig;
    std::shared_ptr<CoordManip> convertor;
    std::shared_ptr<Credits> credits;
    std::vector<std::shared_ptr<MapLayer>> layers;
    std::vector<std::weak_ptr<CameraImpl>> cameras;
    std::vector<std::weak_ptr<SearchTask>> searchTasks;
    std::string authPath;
    std::string mapconfigPath;
    std::string mapconfigView;
    double lastElapsedFrameTime = 0;
    uint32 progressEstimationMaxResources = 0;
    uint32 renderTickIndex = 0;
    bool mapconfigAvailable = false;
    bool mapconfigReady = false;

    MapImpl(Map *map,
            const MapCreateOptions &options,
            const std::shared_ptr<Fetcher> &fetcher);
    ~MapImpl();

    // map api methods
    void setMapconfigPath(const std::string &mapconfigPath,
                          const std::string &authPath);
    void purgeMapconfig();
    void purgeViewCache();

    // rendering
    void renderUpdate(double elapsedTime);
    bool prerequisitesCheck();
    void initializeNavigation();
    std::pair<Validity, std::shared_ptr<GeodataStylesheet>>
        getActualGeoStyle(const std::string &name);
    std::pair<Validity, std::shared_ptr<const std::string>>
        getActualGeoFeatures(const std::string &name,
            const std::string &geoName, float priority);
    std::pair<Validity, std::shared_ptr<const std::string>>
        getActualGeoFeatures(const std::string &name);
    void traverseClearing(TraverseNode *trav);

    // resources methods
    void touchResource(const std::shared_ptr<Resource> &resource);
    Validity getResourceValidity(const std::string &name);
    Validity getResourceValidity(const std::shared_ptr<Resource> &resource);

    std::shared_ptr<GpuTexture> getTexture(const std::string &name);
    std::shared_ptr<GpuAtmosphereDensityTexture>
        getAtmosphereDensityTexture(const std::string &name);
    std::shared_ptr<GpuMesh> getMesh(const std::string &name);
    std::shared_ptr<AuthConfig> getAuthConfig(const std::string &name);
    std::shared_ptr<Mapconfig> getMapconfig(const std::string &name);
    std::shared_ptr<MetaTile> getMetaTile(const std::string &name);
    std::shared_ptr<MeshAggregate> getMeshAggregate(const std::string &name);
    std::shared_ptr<ExternalBoundLayer> getExternalBoundLayer(
            const std::string &name);
    std::shared_ptr<ExternalFreeLayer> getExternalFreeLayer(
            const std::string &name);
    std::shared_ptr<BoundMetaTile> getBoundMetaTile(const std::string &name);
    std::shared_ptr<SearchTaskImpl> getSearchTask(const std::string &name);
    std::shared_ptr<TilesetMapping> getTilesetMapping(const std::string &name);
    std::shared_ptr<GeodataFeatures> getGeoFeatures(const std::string &name);
    std::shared_ptr<GeodataStylesheet> getGeoStyle(const std::string &name);
    std::shared_ptr<GeodataStylesheet> newGeoStyle(const std::string &name,
        const std::string &value);
    std::shared_ptr<GeodataTile> getGeodata(const std::string &name);
    std::shared_ptr<GpuFont> getFont(const std::string &name);

    std::shared_ptr<SearchTask> search(const std::string &query,
                                       const double point[3]);
    void parseSearchResults(const std::shared_ptr<SearchTask> &task);

    void updateSearch();
    double getMapRenderProgress();
    bool getMapRenderComplete();
    TileId roundId(TileId nodeId);
};

std::string convertPath(const std::string &path,
                        const std::string &parent);
std::string convertNameToPath(const std::string &path,
    bool preserveSlashes);
std::string convertNameToFolderAndFile(const std::string &path,
    std::string &folder, std::string &file);

} // namespace vts

#endif
