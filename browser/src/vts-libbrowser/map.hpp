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
#include <boost/thread/mutex.hpp>
#include <vts-libs/vts/urltemplate.hpp>
#include <vts-libs/vts/nodeinfo.hpp>
#include <dbglog/dbglog.hpp>

#include "include/vts-browser/celestial.hpp"
#include "include/vts-browser/statistics.hpp"
#include "include/vts-browser/options.hpp"
#include "include/vts-browser/search.hpp"
#include "include/vts-browser/draws.hpp"
#include "include/vts-browser/math.hpp"

#include "resources.hpp"
#include "credits.hpp"
#include "coordsManip.hpp"
#include "cache.hpp"

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

class BoundParamInfo : public vtslibs::registry::View::BoundLayerParams
{
public:
    typedef std::vector<BoundParamInfo> List;

    BoundParamInfo(const vtslibs::registry::View::BoundLayerParams &params);
    const mat3f uvMatrix() const;
    Validity prepare(const NodeInfo &nodeInfo, class MapImpl *impl,
                 uint32 subMeshIndex, double priority);

    UrlTemplate::Vars orig;
    UrlTemplate::Vars vars;
    MapConfig::BoundInfo *bound;
    int depth;
    bool watertight;
    bool transparent;
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
        vec3 points[2];
        mat4 rotInv;
    };

    struct MetaInfo : public vtslibs::vts::MetaNode
    {
        std::vector<std::shared_ptr<MetaTile>> metaTiles;
        std::vector<vtslibs::registry::CreditId> credits;
        std::shared_ptr<Obb> obb;
        vec3 cornersPhys[8];
        vec3 aabbPhys[2];
        vec3 surrogatePhys;
        const MapConfig::SurfaceStackItem *surface;
        MetaInfo(const vtslibs::vts::MetaNode &node);
    };

    std::shared_ptr<MetaInfo> meta;
    std::vector<std::shared_ptr<TraverseNode>> childs;
    std::vector<RenderTask> opaque;
    std::vector<RenderTask> transparent;
    NodeInfo nodeInfo;
    TraverseNode *const parent;
    uint32 lastAccessTime;
    uint32 lastRenderTime;
    float priority;

    TraverseNode(TraverseNode *parent, const NodeInfo &nodeInfo);
    ~TraverseNode();
    void clearAll();
    void clearRenders();
    bool rendersReady() const;
    bool rendersEmpty() const;
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
    std::string mapConfigPath;
    std::string mapConfigView;
    bool initialized;

    class Navigation
    {
    public:
        std::vector<RenderTask> renders;
        vec3 changeRotation;
        vec3 targetPoint;
        double autoRotation;
        double targetViewExtent;
        boost::optional<double> lastPositionAltitudeShift;
        boost::optional<double> positionAltitudeResetHeight;
        NavigationMode mode;
        NavigationType type;
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
        std::shared_ptr<TraverseNode> traverseRoot;
        std::shared_ptr<TilesetMapping> tilesetMapping;
        mat4 viewProj;
        mat4 viewProjRender;
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
    void pan(const vec3 &value);
    void rotate(const vec3 &value);
    void zoom(double value);
    void setPoint(const vec3 &point);
    void setRotation(const vec3 &euler);
    void setViewExtent(double viewExtent);
    void updatePositionAltitudeShift();
    void resetNavigationMode();
    void convertPositionSubjObj();
    void positionToCamera(vec3 &center, vec3 &dir, vec3 &up);
    double positionObjectiveDistance();
    void initializeNavigation();
    void updateNavigation();
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
    std::shared_ptr<BoundMetaTile> getBoundMetaTile(const std::string &name);
    std::shared_ptr<BoundMaskTile> getBoundMaskTile(const std::string &name);
    std::shared_ptr<SearchTaskImpl> getSearchTask(const std::string &name);
    std::shared_ptr<TilesetMapping> getTilesetMapping(const std::string &name);
    std::shared_ptr<SriIndex> getSriIndex(const std::string &name);
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
    void renderTickPrepare();
    void renderTickRender();
    const TileId roundId(TileId nodeId);
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
    bool travDetermineMeta(TraverseNode *trav);
    bool travDetermineDraws(TraverseNode *trav);
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

} // namespace vts

#endif
