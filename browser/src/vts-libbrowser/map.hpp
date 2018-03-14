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

#ifndef MAP_H_cvukikljqwdf
#define MAP_H_cvukikljqwdf

#include <queue>
#include <array>
#include <boost/thread/mutex.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <vts-libs/vts/urltemplate.hpp>
#include <vts-libs/vts/nodeinfo.hpp>
#include <dbglog/dbglog.hpp>

#include "include/vts-browser/celestial.hpp"
#include "include/vts-browser/statistics.hpp"
#include "include/vts-browser/options.hpp"
#include "include/vts-browser/search.hpp"
#include "include/vts-browser/draws.hpp"
#include "include/vts-browser/math.hpp"
#include "include/vts-browser/exceptions.hpp"

#include "resources/resources.hpp"
#include "resources/cache.hpp"
#include "credits.hpp"
#include "coordsManip.hpp"
#include "utilities/array.hpp"

namespace vts
{

using vtslibs::vts::NodeInfo;
using vtslibs::vts::TileId;
using vtslibs::vts::UrlTemplate;

enum class Validity
{
    Indeterminate,
    Invalid,
    Valid,
};

class MapLayer;

class BoundInfo : public vtslibs::registry::BoundLayer
{
public:
    BoundInfo(const vtslibs::registry::BoundLayer &bl,
              const std::string &url);

    std::shared_ptr<vtslibs::registry::BoundLayer::Availability>
                                    availability;
    UrlTemplate urlExtTex;
    UrlTemplate urlMeta;
    UrlTemplate urlMask;
};

class FreeInfo : public vtslibs::registry::FreeLayer
{
public:
    FreeInfo(const vtslibs::registry::FreeLayer &fl,
              const std::string &url);

    std::string url;

    std::string style() const;
    std::shared_ptr<GeodataStylesheet> stylesheet;
    std::string overrideStyle;
    std::string overrideGeodata; // monolithic only
};

class BoundParamInfo : public vtslibs::registry::View::BoundLayerParams
{
public:
    typedef std::vector<BoundParamInfo> List;

    BoundParamInfo(const vtslibs::registry::View::BoundLayerParams &params);
    mat3f uvMatrix() const;
    Validity prepare(const NodeInfo &nodeInfo, MapImpl *impl,
                     uint32 subMeshIndex, double priority);

    std::shared_ptr<Resource> textureColor;
    std::shared_ptr<Resource> textureMask;
    const BoundInfo *bound;
    bool transparent;

private:
    Validity prepareDepth(MapImpl *impl, double priority);

    UrlTemplate::Vars orig;
    sint32 depth;
};

class SurfaceInfo
{
public:
    SurfaceInfo();
    SurfaceInfo(const vtslibs::vts::SurfaceCommonConfig &surface,
                const std::string &parentPath);
    SurfaceInfo(const vtslibs::registry::FreeLayer::MeshTiles &surface,
                const std::string &parentPath);
    SurfaceInfo(const vtslibs::registry::FreeLayer::GeodataTiles &surface,
                const std::string &parentPath);
    SurfaceInfo(const vtslibs::registry::FreeLayer::Geodata &surface,
                const std::string &parentPath);

    UrlTemplate urlMeta;
    UrlTemplate urlMesh;
    UrlTemplate urlIntTex;
    UrlTemplate urlGeodata;
    vtslibs::vts::TilesetIdList name;
    vec3f color;
    bool alien;
};

class SurfaceStack
{
public:
    void print();
    void colorize();

    void generateVirtual(MapImpl *map,
            const vtslibs::vts::VirtualSurfaceConfig *virtualSurface);
    void generateTileset(MapImpl *map,
            const std::vector<std::string> &vsId,
            const vtslibs::vts::TilesetReferencesList &dataRaw);
    void generateReal(MapImpl *map);
    void generateFree(MapImpl *map, const FreeInfo &freeLayer);

    std::vector<SurfaceInfo> surfaces;
};

class RenderTask
{
public:
    std::shared_ptr<MeshAggregate> meshAgg;
    std::shared_ptr<Resource> mesh;
    std::shared_ptr<Resource> textureColor;
    std::shared_ptr<Resource> textureMask;
    mat4 model;
    mat3f uvm;
    vec4f color;
    bool externalUv;
    bool flatShading;

    RenderTask();
    bool ready() const;
};

class TraverseNode
{
public:
    struct Obb
    {
        mat4 rotInv;
        vec3 points[2];
    };

    // traversal
    Array<std::shared_ptr<TraverseNode>, 4> childs;
    MapLayer *const layer;
    TraverseNode *const parent;
    const NodeInfo nodeInfo;
    const uint32 hash;

    // metadata
    std::vector<std::shared_ptr<MetaTile>> metaTiles;
    boost::optional<vtslibs::vts::MetaNode> meta;
    boost::optional<Obb> obb;
    vec3 cornersPhys[8];
    vec3 aabbPhys[2];
    boost::optional<vec3> surrogatePhys;
    boost::optional<float> surrogateNav;
    const SurfaceInfo *surface;

    uint32 lastAccessTime;
    uint32 lastRenderTime;
    float priority;

    // renders
    std::vector<vtslibs::registry::CreditId> credits;
    std::vector<RenderTask> opaque;
    std::vector<RenderTask> transparent;

    TraverseNode(MapLayer *layer, TraverseNode *parent,
                 const NodeInfo &nodeInfo);
    ~TraverseNode();
    void clearAll();
    void clearRenders();
    bool rendersReady() const;
    bool rendersEmpty() const;
};

class MapLayer
{
public:
    // main surface stack
    MapLayer(MapImpl *map);
    // free layer
    MapLayer(MapImpl *map, const std::string &name,
             const vtslibs::registry::View::FreeLayerParams &params);

    bool prerequisitesCheck();

    BoundParamInfo::List boundList(
            const SurfaceInfo *surface, sint32 surfaceReference);

    vtslibs::registry::View::Surfaces boundLayerParams;

    std::string freeLayerName;
    boost::optional<FreeInfo> freeLayer;
    boost::optional<vtslibs::registry::View::FreeLayerParams> freeLayerParams;

    SurfaceStack surfaceStack;
    boost::optional<SurfaceStack> tilesetStack;

    std::shared_ptr<TraverseNode> traverseRoot;

    MapImpl *const map;

private:
    bool prerequisitesCheckMainSurfaces();
    bool prerequisitesCheckFreeLayer();
};

class MapConfig : public Resource, public vtslibs::vts::MapConfig
{
public:
    class BrowserOptions
    {
    public:
        BrowserOptions();

        std::string searchUrl;
        std::string searchSrs;
        double autorotate;
        bool searchFilter;
    };

    MapConfig(MapImpl *map, const std::string &name);
    void load() override;
    vtslibs::registry::Srs::Type navigationSrsType() const;

    void consolidateView();
    void initializeCelestialBody() const;
    bool isEarth() const;

    BoundInfo *getBoundInfo(const std::string &id);
    FreeInfo *getFreeInfo(const std::string &id);
    vtslibs::vts::SurfaceCommonConfig *findGlue(
            const vtslibs::vts::Glue::Id &id);
    vtslibs::vts::SurfaceCommonConfig *findSurface(
            const std::string &id);

    BrowserOptions browserOptions;

private:
    std::unordered_map<std::string, std::shared_ptr<BoundInfo>> boundInfos;
    std::unordered_map<std::string, std::shared_ptr<FreeInfo>> freeInfos;
};

class MapImpl
{
public:
    MapImpl(class Map *map,
            const MapCreateOptions &options);

    class Map *const map;
    const MapCreateOptions createOptions;
    MapCallbacks callbacks;
    MapStatistics statistics;
    MapOptions options;
    MapDraws draws;
    MapCredits credits;
    MapCelestialBody body;
    std::shared_ptr<MapConfig> mapConfig;
    std::shared_ptr<CoordManip> convertor;
    std::vector<std::shared_ptr<MapLayer>> layers;
    std::string mapConfigPath;
    std::string mapConfigView;
    bool mapconfigAvailable;
    bool mapconfigReady;

    class Navigation
    {
    public:
        std::vector<RenderTask> renders;
        vec3 changeRotation;
        vec3 targetPoint;
        double autoRotation;
        double targetViewExtent;
        boost::optional<double> lastPositionAltitude;
        boost::optional<double> positionAltitudeReset;
        NavigationMode mode;
        NavigationType previousType;

        Navigation();
    } navigation;

    class Resources
    {
    public:
        std::shared_ptr<Cache> cache;
        std::shared_ptr<AuthConfig> auth;
        std::shared_ptr<Fetcher> fetcher;
        std::unordered_map<std::string, std::shared_ptr<Resource>> resources;
        std::vector<std::shared_ptr<Resource>> resourcesCopy;
        std::deque<std::weak_ptr<SearchTask>> searchTasks;
        std::deque<std::shared_ptr<SriIndex>> sriTasks;
        boost::mutex mutResourcesCopy;
        std::string authPath;
        std::string sriPath;
        std::atomic<uint32> downloads;
        uint32 tickIndex;
        uint32 progressEstimationMaxResources;

        Resources();
    } resources;

    class Renderer
    {
    public:
        Credits credits;
        mat4 viewProj;
        mat4 viewProjRender;
        mat4 viewRender;
        vec4 frustumPlanes[6];
        vec3 perpendicularUnitVector;
        vec3 forwardUnitVector;
        vec3 cameraPosPhys;
        vec3 focusPosPhys;
        uint32 windowWidth;
        uint32 windowHeight;
        uint32 tickIndex;
        
        Renderer();
    } renderer;

    // map api methods
    void setMapConfigPath(const std::string &mapConfigPath,
                          const std::string &authPath,
                          const std::string &sriPath);
    void purgeMapConfig();
    void purgeViewCache();
    void printDebugInfo();

    // navigation
    void pan(vec3 value);
    void rotate(vec3 value);
    void zoom(double value);
    void setPoint(const vec3 &point);
    void setRotation(const vec3 &euler);
    void setViewExtent(double viewExtent);
    bool getPositionAltitude(double &result, const vec3 &navPos,
                             double samples);
    void updatePositionAltitude(double fadeOutFactor
                = std::numeric_limits<double>::quiet_NaN());
    void resetNavigationMode();
    void convertPositionSubjObj();
    void positionToCamera(vec3 &center, vec3 &dir, vec3 &up);
    double positionObjectiveDistance();
    void initializeNavigation();
    void updateNavigation(double elapsedTime);
    bool isNavigationModeValid() const;

    // resources methods
    void resourceDataInitialize(const std::shared_ptr<Fetcher> &fetcher);
    void resourceDataFinalize();
    bool resourceDataTick();
    void resourceRenderInitialize();
    void resourceRenderFinalize();
    void resourceRenderTick();
    void touchResource(const std::shared_ptr<Resource> &resource);
    void resourceLoad(const std::shared_ptr<Resource> &resource);
    std::shared_ptr<GpuTexture> getTexture(const std::string &name);
    std::shared_ptr<GpuMesh> getMeshRenderable(
            const std::string &name);
    std::shared_ptr<AuthConfig> getAuthConfig(const std::string &name);
    std::shared_ptr<MapConfig> getMapConfig(const std::string &name);
    std::shared_ptr<MetaTile> getMetaTile(const std::string &name);
    std::shared_ptr<NavTile> getNavTile(const std::string &name);
    std::shared_ptr<MeshAggregate> getMeshAggregate(const std::string &name);
    std::shared_ptr<ExternalBoundLayer> getExternalBoundLayer(
            const std::string &name);
    std::shared_ptr<ExternalFreeLayer> getExternalFreeLayer(
            const std::string &name);
    std::shared_ptr<BoundMetaTile> getBoundMetaTile(const std::string &name);
    std::shared_ptr<SearchTaskImpl> getSearchTask(const std::string &name);
    std::shared_ptr<TilesetMapping> getTilesetMapping(const std::string &name);
    std::shared_ptr<SriIndex> getSriIndex(const std::string &name);
    std::shared_ptr<GeodataFeatures> getGeoFeatures(const std::string &name);
    std::shared_ptr<GeodataStylesheet> getGeoStyle(const std::string &name);
    Validity getResourceValidity(const std::string &name);
    Validity getResourceValidity(const std::shared_ptr<Resource> &resource);
    float computeResourcePriority(TraverseNode *trav);
    std::shared_ptr<SearchTask> search(const std::string &query,
                                       const double point[3]);
    void updateSearch();
    void initiateSri(const vtslibs::registry::Position *position);
    void updateSris();
    double getMapRenderProgress();

    // renderer methods
    void renderInitialize();
    void renderFinalize();
    void renderTickPrepare(double elapsedTime);
    void renderTickRender();
    vtslibs::vts::TileId roundId(TileId nodeId);
    Validity reorderBoundLayers(const NodeInfo &nodeInfo, uint32 subMeshIndex,
                           BoundParamInfo::List &boundList, double priority);
    void touchDraws(const RenderTask &task);
    void touchDraws(const std::vector<RenderTask> &renders);
    void touchDraws(TraverseNode *trav);
    bool visibilityTest(TraverseNode *trav);
    bool coarsenessTest(TraverseNode *trav);
    double coarsenessValue(TraverseNode *trav);
    void renderNode(TraverseNode *trav,
                    const vec4f &uvClip = vec4f(-1,-1,2,2));
    void renderNodePartialRecursive(TraverseNode *trav,
                    vec4f uvClip = vec4f(0,0,1,1));
    std::shared_ptr<Resource> travInternalTexture(TraverseNode *trav,
                                                  uint32 subMeshIndex);
    bool generateMonolithicGeodataTrav(TraverseNode *trav);
    bool travDetermineMeta(TraverseNode *trav);
    void travDetermineMetaImpl(TraverseNode *trav);
    bool travDetermineDraws(TraverseNode *trav);
    bool travDetermineDrawsSurface(TraverseNode *trav);
    bool travDetermineDrawsGeodata(TraverseNode *trav);
    double travDistance(TraverseNode *trav, const vec3 pointPhys);
    bool travInit(TraverseNode *trav, bool skipStatistics = false);
    void travModeHierarchical(TraverseNode *trav, bool loadOnly);
    void travModeFlat(TraverseNode *trav);
    void travModeBalanced(TraverseNode *trav);
    void traverseRender(TraverseNode *trav);
    void traverseClearing(TraverseNode *trav);
    void updateCamera();
    bool prerequisitesCheck();
    void applyCameraRotationNormalization(vec3 &rot);
};

bool testAndThrow(Resource::State state, const std::string &message);
std::string convertPath(const std::string &path,
                        const std::string &parent);

} // namespace vts

#endif
