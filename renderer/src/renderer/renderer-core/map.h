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

#include <renderer/statistics.h>
#include <renderer/gpuResources.h>
#include <renderer/gpuContext.h>
#include <renderer/buffer.h>

#include "csConvertor.h"
#include "mapConfig.h"
#include "resource.h"
#include "resourceMap.h"
#include "math.h"

namespace melown
{

using vtslibs::vts::NodeInfo;
using vtslibs::vts::TileId;
using vtslibs::vts::MetaNode;
using vtslibs::vts::UrlTemplate;

class HeightRequest
{
public:
    static const NodeInfo findPosition(NodeInfo &info,
                                       const vec2 &pos,
                                       double viewExtent);
    
    HeightRequest(const vec2 &navPos, class MapImpl *map);
    
    std::string navUrl;
    std::string metaUrl;
    TileId nodeId;
    vec2 pixPos;
    uint32 frameIndex;
};

class BoundParamInfo : public vtslibs::registry::View::BoundLayerParams
{
public:
    typedef std::vector<BoundParamInfo> list;

    BoundParamInfo(const vtslibs::registry::View::BoundLayerParams &params);
    bool available(bool primaryLod = false);
    void varsFallDown(int depth);
    const mat3f uvMatrix() const;
    bool canGoUp() const;
    int depth() const;
    void prepare(const NodeInfo &nodeInfo, class MapImpl *impl,
                 uint32 subMeshIndex);
    bool invalid() const;
    bool operator < (const BoundParamInfo &rhs) const;
    
    UrlTemplate::Vars orig;
    UrlTemplate::Vars vars;
    const NodeInfo *nodeInfo;
    MapImpl *impl;
    BoundInfo *bound;
    bool watertight;
    
private:
    bool availCache;
};

class MapImpl
{
public:
    MapImpl(const std::string &mapConfigPath);

    std::shared_ptr<CsConvertor> convertor;
    std::string mapConfigPath;
    MapStatistics statistics;
    GpuContext *renderContext;
    GpuContext *dataContext;   
    MapConfig *mapConfig;
    
    class Resources
    {
    public:
        std::unordered_map<std::string, std::shared_ptr<Resource>> resources;
        std::unordered_set<ResourceImpl*> prepareQue;
        std::unordered_set<ResourceImpl*> prepareQueNew;
        std::unordered_set<std::string> invalidUrl;
        std::unordered_set<std::string> invalidUrlNew;
        boost::mutex mutPrepareQue;
        boost::mutex mutInvalidUrls;
        Fetcher *fetcher;
        uint32 takeItemIndex;
        uint32 tickIndex;
        std::atomic_uint downloads;
        bool destroyTheFetcher;
        static const std::string invalidUrlFileName;
        
        Resources();
        ~Resources();
    } resources;
    
    class Renderer
    {
    public:
        vec3 perpendicularUnitVector;
        vec3 forwardUnitVector;
        mat4 viewProj;
        uint32 windowWidth;
        uint32 windowHeight;
        GpuShader *shader;
        GpuShader *shaderColor;
        uint32 metaTileBinaryOrder;
        boost::optional<double> lastPanZShift;
        std::queue<std::shared_ptr<HeightRequest>> panZQueue;
        
        Renderer();
    } renderer;
    
    void pan(const vec3 &value);
    void rotate(const vec3 &value); 
    
    void dataInitialize(GpuContext *context, Fetcher *fetcher);
    void dataFinalize();
    bool dataTick();
    void dataRenderInitialize();
    void dataRenderFinalize();
    bool dataRenderTick();
    void loadResource(ResourceImpl *r);
    void fetchedFile(FetchTask *task);
    void touchResource(const std::string &, Resource *resource);
    GpuShader *getShader(const std::string &name);
    GpuTexture *getTexture(const std::string &name);
    GpuMeshRenderable *getMeshRenderable(const std::string &name);
    MapConfig *getMapConfig(const std::string &name);
    MetaTile *getMetaTile(const std::string &name);
    NavTile *getNavTile(const std::string &name);
    MeshAggregate *getMeshAggregate(const std::string &name);
    ExternalBoundLayer *getExternalBoundLayer(const std::string &name);
    BoundMetaTile *getBoundMetaTile(const std::string &name);
    BoundMaskTile *getBoundMaskTile(const std::string &name);
    bool getResourceReady(const std::string &name);
    
    void renderInitialize(GpuContext *context);
    void renderFinalize();
    void renderTick(uint32 windowWidth, uint32 windowHeight);
    const TileId roundId(TileId nodeId);
    void reorderBoundLayer(const NodeInfo &nodeInfo, uint32 subMeshIndex,
                           BoundParamInfo::list &boundList);
    void drawBox(const mat4f &mvp, const vec3f &color = vec3f(1, 0, 0));
    bool renderNode(const NodeInfo &nodeInfo,
                    const SurfaceStackItem &surface,
                    bool onlyCheckReady);
    const bool visibilityTest(const NodeInfo &nodeInfo, const MetaNode &node);
    bool coarsenessTest(const NodeInfo &nodeInfo, const MetaNode &node);
    void traverse(const NodeInfo &nodeInfo);
    bool panZSurfaceStack(HeightRequest &task);
    void checkPanZQueue();
    void updateCamera();
    const bool prerequisitesCheck();
    void mapConfigLoaded();
    void panAdjustZ(const vec2 &navPos);
    BoundInfo *getBoundInfo(const std::string &id);
    
    template<class T,
        std::shared_ptr<Resource>(GpuContext::*F)(const std::string &)>
    T *getGpuResource(const std::string &name)
    {
        auto it = resources.resources.find(name);
        if (it == resources.resources.end())
        {
            resources.resources[name] = (renderContext->*F)(name);
            it = resources.resources.find(name);
        }
        touchResource(name, it->second.get());
        return dynamic_cast<T*>(it->second.get());
    }
    
    template<class T> T *getMapResource(const std::string &name)
    {
        auto it = resources.resources.find(name);
        if (it == resources.resources.end())
        {
            resources.resources[name] = std::shared_ptr<Resource>(new T(name));
            it = resources.resources.find(name);
        }
        touchResource(name, it->second.get());
        return dynamic_cast<T*>(it->second.get());
    }
};

} // namespace melown

#endif
