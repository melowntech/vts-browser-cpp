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
using vtslibs::vts::MetaNode;
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
    bool transparent;
    
    RenderTask();
    bool ready() const;
};

class TraverseNode
{
public:
    NodeInfo nodeInfo;
    vec3 cornersPhys[8];
    vec3 aabbPhys[2];
    vec3 surrogatePhys;
    std::vector<std::shared_ptr<RenderTask>> draws;
    std::vector<std::shared_ptr<TraverseNode>> childs;
    std::vector<vtslibs::registry::CreditId> credits;
    const MapConfig::SurfaceStackItem *surface;
    uint32 lastAccessTime;
    uint32 flags;
    float texelSize;
    float surrogateValue;
    uint16 displaySize;
    Validity validity;
    bool empty;

    static std::atomic<sint32> instanceCounter;

    TraverseNode(const NodeInfo &nodeInfo);
    ~TraverseNode();
    void clear();
    bool ready() const;
    double distancePhys(const vec3 &point) const;
};

class TraverseQueueItem
{
public:
    TraverseQueueItem(const std::shared_ptr<TraverseNode> &trav,
                      float priority);
    
    std::shared_ptr<TraverseNode> trav;
    float priority;
    
    bool operator < (const TraverseQueueItem &other) const;
};

class MapImpl
{
public:
    MapImpl(class Map *map,
            const class MapCreateOptions &options);

    class Map *const map;
    std::shared_ptr<MapConfig> mapConfig;
    std::shared_ptr<CoordManip> convertor;
    std::shared_ptr<AuthConfig> auth;
    std::shared_ptr<Cache> cache;
    std::string mapConfigPath;
    std::string mapConfigView;
    std::string authPath;
    std::string sriPath;
    std::string clientId;
    MapCallbacks callbacks;
    MapStatistics statistics;
    MapOptions options;
    MapDraws draws;
    MapCredits credits;
    bool initialized;

    class Navigation
    {
    public:
        vec3 changeRotation;
        vec3 targetPoint;
        double autoRotation;
        double targetViewExtent;
        std::queue<std::shared_ptr<class HeightRequest>> panZQueue;
        boost::optional<double> lastPanZShift;
        NavigationGeographicMode geographicMode;
        NavigationType type;
        
        Navigation();
    } navigation;

    class Resources
    {
    public:
        std::unordered_map<std::string, std::shared_ptr<Resource>> resources;
        std::vector<std::shared_ptr<Resource>> resourcesCopy;
        boost::mutex mutResourcesCopy;

        std::atomic_uint downloads;
        std::shared_ptr<Fetcher> fetcher;
        std::deque<std::weak_ptr<SearchTask>> searchTasks;
        
        Resources();
    } resources;

    class Renderer
    {
    public:
        Credits credits;
        std::shared_ptr<TraverseNode> traverseRoot;
        std::priority_queue<TraverseQueueItem> traverseQueue;
        mat4 viewProj;
        mat4 viewProjRender;
        vec4 frustumPlanes[6];
        vec3 perpendicularUnitVector;
        vec3 forwardUnitVector;
        vec3 cameraPosPhys;
        vec3 focusPosPhys;
        uint32 windowWidth;
        uint32 windowHeight;
        
        Renderer();
    } renderer;

    // map api methods
    void setMapConfigPath(const std::string &mapConfigPath,
                          const std::string &authPath,
                          const std::string &sriPath);
    void purgeMapConfig();
    void purgeTraverseCache();
    void printDebugInfo();

    // navigation
    void pan(const vec3 &value);
    void rotate(const vec3 &value);
    void zoom(double value);
    void setPoint(const vec3 &point, NavigationType type);
    void setRotation(const vec3 &euler, NavigationType type);
    void setViewExtent(double viewExtent, NavigationType type);
    void checkPanZQueue();
    const std::pair<vtslibs::vts::NodeInfo, vec2> findInfoNavRoot(
            const vec2 &navPos);
    const NodeInfo findInfoSdsSampled(const NodeInfo &info,
                                      const vec2 &sdsPos);
    void resetPositionAltitude(double resetOffset);
    void resetNavigationGeographicMode();
    void convertPositionSubjObj();
    void positionToCamera(vec3 &center, vec3 &dir, vec3 &up);
    double positionObjectiveDistance();
    void initializeNavigation();
    void updateNavigation();

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
    Validity getResourceValidity(const std::string &name);
    Validity getResourceValidity(const std::shared_ptr<Resource> &resource);
    float computeResourcePriority(const std::shared_ptr<TraverseNode> &trav);

    // renderer methods
    void renderInitialize();
    void renderFinalize();
    void renderTickPrepare();
    void renderTickRender(uint32 windowWidth, uint32 windowHeight);
    const TileId roundId(TileId nodeId);
    Validity reorderBoundLayers(const NodeInfo &nodeInfo, uint32 subMeshIndex,
                           BoundParamInfo::List &boundList, double priority);
    void touchResources(const std::shared_ptr<TraverseNode> &trav);
    void touchResources(const std::shared_ptr<RenderTask> &task);
    bool visibilityTest(const std::shared_ptr<TraverseNode> &trav);
    bool coarsenessTest(const std::shared_ptr<TraverseNode> &trav);
    Validity returnValidMetaNode(MapConfig::SurfaceInfo *surface,
                const TileId &nodeId, const MetaNode *&node, double priority);
    Validity checkMetaNode(MapConfig::SurfaceInfo *surface,
                const TileId &nodeId, const MetaNode *&node, double priority);
    void renderNode(const std::shared_ptr<TraverseNode> &trav);
    void traverseValidNode(const std::shared_ptr<TraverseNode> &trav);
    bool traverseDetermineSurface(const std::shared_ptr<TraverseNode> &trav);
    bool traverseDetermineBoundLayers(
            const std::shared_ptr<TraverseNode> &trav);
    void traverse(const std::shared_ptr<TraverseNode> &trav, bool loadOnly);
    void traverseClearing(const std::shared_ptr<TraverseNode> &trav);
    void updateCamera();
    bool prerequisitesCheck();
    void emptyTraverseQueue();

    // search
    std::shared_ptr<SearchTask> search(const std::string &query,
                                       const double point[3]);
    void updateSearch();
};

} // namespace vts

#endif
