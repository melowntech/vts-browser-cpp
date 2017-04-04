#ifndef MAP_H_cvukikljqwdf
#define MAP_H_cvukikljqwdf

#include <string>
#include <memory>
#include <queue>
#include <atomic>
#include <unordered_map>
#include <unordered_set>

#include <boost/thread/mutex.hpp>
#include <boost/optional/optional_io.hpp>
#include <vts-libs/vts/urltemplate.hpp>
#include <vts-libs/vts/nodeinfo.hpp>
#include <dbglog/dbglog.hpp>
#include <utility/uri.hpp>

#include <melown/statistics.hpp>
#include <melown/options.hpp>
#include <melown/rendering.hpp>
#include <melown/resources.hpp>
#include <melown/buffer.hpp>

#include "mapConfig.hpp"
#include "resource.hpp"
#include "resourceMap.hpp"
#include "math.hpp"

namespace melown
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

class HeightRequest
{
public:
    static const NodeInfo findPosition(NodeInfo &info,
                                       const vec2 &pos,
                                       class MapImpl *map);
    
    HeightRequest(class MapImpl *map);
    
    boost::optional<NodeInfo> nodeInfo;
    SurfaceStackItem *surface;
    vec2 sds;
    uint32 frameIndex;
};

class BoundParamInfo : public vtslibs::registry::View::BoundLayerParams
{
public:
    typedef std::vector<BoundParamInfo> list;

    BoundParamInfo(const vtslibs::registry::View::BoundLayerParams &params);
    const mat3f uvMatrix() const;
    Validity prepare(const NodeInfo &nodeInfo, class MapImpl *impl,
                 uint32 subMeshIndex);
    bool operator < (const BoundParamInfo &rhs) const;
    
    UrlTemplate::Vars orig;
    UrlTemplate::Vars vars;
    BoundInfo *bound;
    int depth;
    bool watertight;
};

class RenderTask
{
public:
    std::shared_ptr<MeshAggregate> meshAgg;
    std::shared_ptr<GpuMesh> mesh;
    std::shared_ptr<GpuTexture> textureColor;
    std::shared_ptr<GpuTexture> textureMask;
    mat4 model;
    mat3f uvm;
    vec3f color;
    bool external;
    
    RenderTask();
    bool ready() const;
};

class TraverseNode
{
public:
    std::vector<std::shared_ptr<RenderTask>> opaque;
    std::vector<std::shared_ptr<RenderTask>> transparent;
    std::vector<std::shared_ptr<RenderTask>> wires;
    std::vector<std::shared_ptr<RenderTask>> surrogate;
    std::vector<std::shared_ptr<TraverseNode>> childs;
    NodeInfo nodeInfo;
    MetaNode metaNode;
    const SurfaceStackItem *surface;
    uint32 lastAccessTime;
    Validity validity;
    bool empty;
    
    TraverseNode(const NodeInfo &nodeInfo);
    ~TraverseNode();
    void clear();
    bool ready() const;
};

class MapImpl
{
public:
    MapImpl(class MapFoundation *const mapFoundation);

    class MapFoundation *const mapFoundation;
    std::shared_ptr<MapConfig> mapConfig;
    std::string mapConfigPath;
    MapStatistics statistics;
    MapOptions options;
    DrawBatch draws;
    bool initialized;
    
    class Resources
    {
    public:
        static const std::string invalidUrlFileName;
        
        std::unordered_map<std::string, std::shared_ptr<Resource>> resources;
        std::unordered_set<ResourceImpl*> prepareQue;
        std::unordered_set<ResourceImpl*> prepareQueNew;
        std::unordered_set<std::string> invalidUrl;
        std::unordered_set<std::string> invalidUrlNew;
        boost::mutex mutPrepareQue;
        boost::mutex mutInvalidUrls;
        std::atomic_uint downloads;
        Fetcher *fetcher;
        uint32 takeItemIndex;
        bool destroyTheFetcher;
        
        Resources();
        ~Resources();
    } resources;
    
    class Renderer
    {
    public:
        std::vector<std::shared_ptr<RenderTask>> helperRenders;
        std::shared_ptr<TraverseNode> traverseRoot;
        std::queue<std::shared_ptr<HeightRequest>> panZQueue;
        boost::optional<double> lastPanZShift;
        mat4 viewProj;
        vec3 perpendicularUnitVector;
        vec3 forwardUnitVector;
        uint32 windowWidth;
        uint32 windowHeight;
        uint32 metaTileBinaryOrder;
        vec3 inertiaMotion;
        vec3 inertiaRotation;
        double inertiaViewExtent;
        
        Renderer();
    } renderer;
    
    // map foundation methods
    void setMapConfig(const std::string &mapConfigPath);
    void pan(const vec3 &value);
    void rotate(const vec3 &value); 
    
    // resources methods
    void dataInitialize(Fetcher *fetcher);
    void dataFinalize();
    bool dataTick();
    void dataRenderInitialize();
    void dataRenderFinalize();
    bool dataRenderTick();
    void loadResource(ResourceImpl *r);
    void fetchedFile(FetchTask *task);
    void touchResource(std::shared_ptr<Resource> resource);
    std::shared_ptr<GpuTexture> getTexture(const std::string &name);
    std::shared_ptr<GpuMesh> getMeshRenderable(
            const std::string &name);
    std::shared_ptr<MapConfig> getMapConfig(const std::string &name);
    std::shared_ptr<MetaTile> getMetaTile(const std::string &name);
    std::shared_ptr<NavTile> getNavTile(const std::string &name);
    std::shared_ptr<MeshAggregate> getMeshAggregate(const std::string &name);
    std::shared_ptr<ExternalBoundLayer> getExternalBoundLayer(
            const std::string &name);
    std::shared_ptr<BoundMetaTile> getBoundMetaTile(const std::string &name);
    std::shared_ptr<BoundMaskTile> getBoundMaskTile(const std::string &name);
    Validity getResourceValidity(const std::string &name);
    
    // renderer methods
    void renderInitialize();
    void renderFinalize();
    void renderTick(uint32 windowWidth, uint32 windowHeight);
    const TileId roundId(TileId nodeId);
    Validity reorderBoundLayers(const NodeInfo &nodeInfo, uint32 subMeshIndex,
                           BoundParamInfo::list &boundList);
    void touchResources(std::shared_ptr<TraverseNode> trav);
    void touchResources(std::shared_ptr<RenderTask> task);
    bool visibilityTest(const NodeInfo &nodeInfo, const MetaNode &node);
    bool backTileCulling(const NodeInfo &nodeInfo);
    bool coarsenessTest(const NodeInfo &nodeInfo, const MetaNode &node);
    void convertRenderTasks(std::vector<DrawTask> &draws,
                            std::vector<std::shared_ptr<RenderTask>> &renders);
    void traverseValidNode(std::shared_ptr<TraverseNode> &trav);
    bool traverseDetermineSurface(std::shared_ptr<TraverseNode> &trav);
    bool traverseDetermineBoundLayers(std::shared_ptr<TraverseNode> &trav);
    void traverse(std::shared_ptr<TraverseNode> &trav, bool loadOnly);
    void traverseClearing(std::shared_ptr<TraverseNode> &trav);
    void checkPanZQueue();
    void updateCamera();
    bool prerequisitesCheck();
};

} // namespace melown

#endif
