#include <unordered_map>

#include <boost/optional/optional_io.hpp>
#include <vts-libs/vts/urltemplate.hpp>
#include <vts-libs/vts/nodeinfo.hpp>
#include <dbglog/dbglog.hpp>
#include <utility/uri.hpp>

#include <renderer/gpuResources.h>
#include <renderer/statistics.h>

#include "map.h"
#include "mapConfig.h"
#include "renderer.h"
#include "resourceManager.h"
#include "mapResources.h"
#include "csConvertor.h"

namespace melown
{

using namespace vtslibs::vts;
using namespace vtslibs::registry;

class RendererImpl : public Renderer
{
public:
    class SurfaceInfo : public SurfaceConfig
    {
    public:
        SurfaceInfo(const SurfaceConfig &surface, RendererImpl *renderer)
            : SurfaceConfig(surface)
        {
            urlMeta.parse(renderer->convertPath(urls3d->meta,
                                      renderer->mapConfig->name));
            urlMesh.parse(renderer->convertPath(urls3d->mesh,
                                      renderer->mapConfig->name));
            urlIntTex.parse(renderer->convertPath(urls3d->texture,
                                        renderer->mapConfig->name));
        }

        UrlTemplate urlMeta;
        UrlTemplate urlMesh;
        UrlTemplate urlIntTex;
    };

    class BoundInfo : public BoundLayer
    {
    public:
        BoundInfo(const BoundLayer &layer, RendererImpl *)
            : BoundLayer(layer)
        {
            urlExtTex.parse(url);
        }

        UrlTemplate urlExtTex;
    };

    RendererImpl(MapImpl *map) : map(map), mapConfig(nullptr),
        shader(nullptr)
    {}

    void renderInitialize() override
    {}

    const View::BoundLayerParams pickBoundLayer(
            const View::BoundLayerParams::list &boundList)
    {
        // todo - pick first bound layer for now
        return *boundList.begin();
    }

    View::BoundLayerParams::list mergeBoundList(
            const View::BoundLayerParams::list &original,
            uint32 textureLayer)
    {
        View::BoundLayerParams::list res(original);
        res.push_back(View::BoundLayerParams(
                      mapConfig->boundLayers.get(textureLayer).id));
        return res;
    }

    void renderNode(const NodeInfo nodeInfo,
                    const SurfaceInfo &surface,
                    const View::BoundLayerParams::list &boundList)
    {
        // nodeId
        const TileId nodeId = nodeInfo.nodeId();

        // statistics
        map->statistics->meshesRenderedTotal++;
        map->statistics->meshesRenderedPerLod[
                std::min<uint32>(nodeId.lod, MapStatistics::MaxLods-1)]++;

        // aggregate mesh
        MeshAggregate *meshAgg = map->resources->getMeshAggregate
                (surface.urlMesh(UrlTemplate::Vars(nodeId)));
        if (!meshAgg || meshAgg->state != Resource::State::ready)
            return;

        // iterate over all submeshes
        for (uint32 subMeshIndex = 0, e = meshAgg->submeshes.size();
             subMeshIndex != e; subMeshIndex++)
        {
            UrlTemplate::Vars vars(nodeId, local(nodeInfo), subMeshIndex);
            const MeshPart &part = meshAgg->submeshes[subMeshIndex];
            GpuMeshRenderable *mesh = part.renderable.get();
            GpuTexture *texture = nullptr;
            int mode = 0;
            if (part.internalUv)
                texture = map->resources->getTexture(surface.urlIntTex(vars));
            else if (part.externalUv)
            { // bound layer
                const View::BoundLayerParams boundParams = pickBoundLayer(
                            part.textureLayer
                            ? mergeBoundList(boundList, part.textureLayer)
                            : boundList);
                const BoundInfo *bound = getBoundInfo(boundParams.id);
                if (bound)
                {
                    mode = 1;
                    texture = map->resources->getTexture(
                                bound->urlExtTex(vars));
                }
            }
            if (!texture || texture->state != Resource::State::ready)
                texture = map->resources->getTexture("data/helper.jpg");
            if (!texture || texture->state != Resource::State::ready)
                continue;
            texture->bind();
            mat4f mvp = (viewProj * part.normToPhys).cast<float>();
            mat3f uvm = upperLeftSubMatrix(identityMatrix()).cast<float>();
            shader->uniformMat4(0, mvp.data());
            shader->uniformMat3(4, uvm.data());
            shader->uniform(8, mode);
            mesh->draw();
        }
    }

    const std::string &pickSurface(
        const View::BoundLayerParams::list *&boundList)
    {
        // todo - pick first surface for now
        auto it = mapConfig->view.surfaces.begin();
        boundList = &it->second;
        return it->first;
    }

    const bool visibilityTest()
    {
        return true; // todo - all is visible, for now
    }

    const bool coarsenessTest(const NodeInfo &nodeInfo)
    {
        return nodeInfo.nodeId().lod >= 17;
        //return true; // todo - the most rough surfaces are rendered for now
    }

    void traverse(const NodeInfo nodeInfo)
    {
        // nodeId
        const TileId nodeId = nodeInfo.nodeId();

        // statistics
        map->statistics->metaNodesTraverzedTotal++;
        map->statistics->metaNodesTraverzedPerLod[
                std::min<uint32>(nodeId.lod, MapStatistics::MaxLods-1)]++;

        // pick surface
        const View::BoundLayerParams::list *boundList = nullptr;
        const SurfaceInfo *surface = getSurfaceInfo(pickSurface(boundList));
        if (!surface)
            return;

        // pick meta tile
        const TileId tileId(nodeId.lod,
                            (nodeId.x >> binaryOrder) << binaryOrder,
                            (nodeId.y >> binaryOrder) << binaryOrder);
        const MetaTile *tile = map->resources->getMetaTile
                (surface->urlMeta(UrlTemplate::Vars(tileId)));
        if (!tile || tile->state != Resource::State::ready)
            return;

        // pick meta node
        const MetaNode *node = tile->get(nodeId, std::nothrow_t());
        if (!node)
            return;

        if (!visibilityTest())
            return;

        if (node->geometry() && coarsenessTest(nodeInfo))
        { // render the node's resources
            renderNode(nodeInfo, *surface, *boundList);
            return;
        }

        // traverse children
        Children childs = children(nodeId);
        for (uint32 i = 0; i < 4; i++)
            traverse(nodeInfo.child(childs[i]));
    }

    void updateCamera(uint32 width, uint32 height)
    {
        if (mapConfig->position.type
                != vtslibs::registry::Position::Type::objective)
            throw "unsupported position type"; // todo
        if (mapConfig->position.heightMode
                != vtslibs::registry::Position::HeightMode::fixed)
            throw "unsupported position height mode"; // todo

        vec3 center = vecFromUblas<vec3>(mapConfig->position.position);
        vec3 rot = vecFromUblas<vec3>(mapConfig->position.orientation);

        vec3 dir(1, 0, 0);
        vec3 up(0, 0, -1);

        { // apply rotation
            mat3 tmp = upperLeftSubMatrix(rotationMatrix(2, degToRad(-rot(0))))
                    * upperLeftSubMatrix(rotationMatrix(1, degToRad(-rot(1))));
            dir = tmp * dir;
            up = tmp * up;
        }

        switch (mapConfig->srs.get
                (mapConfig->referenceFrame.model.navigationSrs).type)
        {
        case vtslibs::registry::Srs::Type::projected:
        {
            // swap XY
            std::swap(dir(0), dir(1));
            std::swap(up(0), up(1));
            // invert Z
            dir(2) *= -1;
            up(2) *= -1;
            // add center of orbit (transform to navigation srs)
            dir += center;
            up += center;
            // transform to physical srs
            center = map->convertor->navToPhys(center);
            dir = map->convertor->navToPhys(dir);
            up = map->convertor->navToPhys(up);
            // points -> vectors
            dir = normalize(dir - center);
            up = normalize(up - center);
        } break;
        case vtslibs::registry::Srs::Type::geographic:
        {
            // find lat-lon coordinates of points moved to north and east
            vec3 n2 = map->convertor->navGeodesicDirect(center, 0, 100);
            vec3 e2 = map->convertor->navGeodesicDirect(center, 90, 100);
            // transform to physical srs
            center = map->convertor->navToPhys(center);
            vec3 n = map->convertor->navToPhys(n2);
            vec3 e = map->convertor->navToPhys(e2);
            // points -> vectors
            n = normalize(n - center);
            e = normalize(e - center);
            // construct NED coordinate system
            vec3 d = normalize(cross(n, e));
            e = normalize(cross(n, d));
            mat3 tmp = (mat3() << n, e, d).finished();
            // rotate original vectors
            dir = tmp * dir;
            up = tmp * up;
            dir = normalize(dir);
            up = normalize(up);
        } break;
        default:
            throw "not implemented navigation srs type";
        }

        double dist = mapConfig->position.verticalExtent
                * 0.5 / tan(degToRad(mapConfig->position.verticalFov * 0.5));
        mat4 view = lookAt(center - dir * dist, center, up);
        mat4 proj = perspectiveMatrix(mapConfig->position.verticalFov,
                                      (double)width / (double)height,
                                      dist * 0.1, dist * 10);
        viewProj = proj * view;
    }

    const std::string convertPath(const std::string &path,
                                  const std::string &parent)
    {
        return utility::Uri(parent).resolve(path).str();
    }

    const bool configLoaded()
    {
        mapConfig = map->resources->getMapConfig(map->mapConfigPath);
        if (!mapConfig || mapConfig->state != Resource::State::ready)
            return false;

        bool ok = true;
        for (auto &&bl : mapConfig->boundLayers)
        {
            if (!bl.external())
                continue;
            std::string url = convertPath(bl.url, mapConfig->name);
            ExternalBoundLayer *r
                    = map->resources->getExternalBoundLayer(url);
            if (!r || r->state != Resource::State::ready)
                ok = false;
            else
            {
                r->id = bl.id;
                r->url = convertPath(r->url, url);
                if (r->metaUrl)
                    r->metaUrl = convertPath(*r->metaUrl, url);
                if (r->maskUrl)
                    r->maskUrl = convertPath(*r->maskUrl, url);
                if (r->creditsUrl)
                    r->creditsUrl = convertPath(*r->creditsUrl, url);
                mapConfig->boundLayers.replace(*r);
            }
        }
        return ok;
    }

    void onceInitialize()
    {
        map->convertor = std::shared_ptr<CsConvertor>(CsConvertor::create(
            mapConfig->referenceFrame.model.physicalSrs,
            mapConfig->referenceFrame.model.navigationSrs,
            mapConfig->referenceFrame.model.publicSrs,
            *mapConfig
        ));

        binaryOrder = mapConfig->referenceFrame.metaBinaryOrder;

        for (auto &&bl : mapConfig->boundLayers)
        {
            boundInfos[bl.id] = std::shared_ptr<BoundInfo>
                    (new BoundInfo(bl, this));
        }

        for (auto &&sr : mapConfig->surfaces)
        {
            surfaceInfos[sr.id] = std::shared_ptr<SurfaceInfo>
                    (new SurfaceInfo(sr, this));
        }

        // todo - temporarily reset camera
        mapConfig->position.orientation = math::Point3(0, -90, 0);

        LOG(info3) << "position: " << mapConfig->position.position;
        LOG(info3) << "rotation: " << mapConfig->position.orientation;

        for (auto &&bl : mapConfig->boundLayers)
        {
            LOG(info3) << bl.type;
            LOG(info3) << bl.id;
            LOG(info3) << bl.url;
            LOG(info3) << bl.metaUrl;
            LOG(info3) << bl.maskUrl;
            LOG(info3) << bl.creditsUrl;
        }
    }

    void renderTick(uint32 width, uint32 height) override
    {
        if (!configLoaded())
            return;

        shader = map->resources->getShader("data/shaders/a");
        if (!shader || shader->state != Resource::State::ready)
            return;
        shader->bind();

        if (!map->convertor)
            onceInitialize();

        updateCamera(width, height);

        NodeInfo nodeInfo(mapConfig->referenceFrame, TileId(), mapConfig);
        traverse(nodeInfo);
    }

    void renderFinalize() override
    {}

    SurfaceInfo *getSurfaceInfo(const std::string &id)
    {
        auto it = surfaceInfos.find(id);
        if (it == surfaceInfos.end())
            return nullptr;
        return it->second.get();
    }

    BoundInfo *getBoundInfo(const std::string &id)
    {
        auto it = boundInfos.find(id);
        if (it == boundInfos.end())
            return nullptr;
        return it->second.get();
    }

    std::unordered_map<std::string, std::shared_ptr<SurfaceInfo>> surfaceInfos;
    std::unordered_map<std::string, std::shared_ptr<BoundInfo>> boundInfos;

    mat4 viewProj;
    MapImpl *map;
    MapConfig *mapConfig;
    GpuShader *shader;
    uint32 binaryOrder;
};

Renderer::Renderer()
{}

Renderer::~Renderer()
{}

Renderer *Renderer::create(MapImpl *map)
{
    return new RendererImpl(map);
}

} // namespace melown
