#include <renderer/gpuResources.h>

#include "map.h"
#include "mapConfig.h"
#include "renderer.h"
#include "resourceManager.h"
#include "mapResources.h"
#include "csConvertor.h"

#include "../../vts-libs/vts/urltemplate.hpp"
#include "../../dbglog/dbglog.hpp"

namespace melown
{
    using namespace vadstena::vts;
    using namespace vadstena::registry;

    class RendererImpl : public Renderer
    {
    public:
        RendererImpl(MapImpl *map) : map(map), mapConfig(nullptr), surfConf(nullptr), shader(nullptr)
        {}

        void renderInitialize() override
        {
        }

        void traverse(const TileId nodeId)
        {
            const TileId tileId(nodeId.lod, (nodeId.x >> binaryOrder) << binaryOrder, (nodeId.y >> binaryOrder) << binaryOrder);
            const MetaTile *tile = map->resources->getMetaTile(metaUrlTemplate(UrlTemplate::Vars(tileId)));
            if (!tile || tile->state != Resource::State::ready)
                return;
            const MetaNode *node = tile->get(nodeId, std::nothrow_t());
            if (!node)
                return;

            if (nodeId.lod <= targetLod)
            { // traverse children
                if (node->ulChild() ) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 0, nodeId.y * 2 + 0));
                if (node->urChild() ) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 1, nodeId.y * 2 + 0));
                if (node->llChild() ) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 0, nodeId.y * 2 + 1));
                if (node->lrlChild()) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 1, nodeId.y * 2 + 1));
            }

            if (node->geometry() && nodeId.lod == targetLod)
            {
                MeshAggregate *meshAgg = map->resources->getMeshAggregate(meshUrlTemplate(UrlTemplate::Vars(nodeId)));
                if (meshAgg && meshAgg->state == Resource::State::ready)
                {
                    for (uint32 i = 0, e = meshAgg->submeshes.size(); i != e; i++)
                    {
                        MeshPart &part = meshAgg->submeshes[i];
                        GpuMeshRenderable *mesh = part.renderable.get();
                        GpuTexture *texture = map->resources->getTexture(textureInternalUrlTemplate(UrlTemplate::Vars(nodeId).addSubmesh(i)));
                        if (texture && texture->state == Resource::State::errorDownload)
                            texture = map->resources->getTexture("data/helper.jpg");
                        if (texture && texture->state == Resource::State::ready)
                        {
                            texture->bind();
                            mat4f mvp = (viewProj * part.normToPhys).cast<float>();
                            shader->uniformMat4(0, mvp.data());
                            mesh->draw();

                            lastRenderable = vec4to3(part.normToPhys * vec4(0, 0, 0, 1));
                            sumRenderable += lastRenderable;
                            countRenderale++;
                        }
                    }
                }
            }
        }

        void updateCamera(uint32 width, uint32 height)
        {
            if (mapConfig->position.type != vadstena::registry::Position::Type::objective)
                throw "unsupported position type"; // todo
            if (mapConfig->position.heightMode != vadstena::registry::Position::HeightMode::fixed)
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

            switch (mapConfig->srs.get(mapConfig->referenceFrame.model.navigationSrs).type)
            {
            case vadstena::registry::Srs::Type::projected:
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
            case vadstena::registry::Srs::Type::geographic:
            {
                // find lat-lon coordinates of points moved to north and east respectively
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
                // convert original vectors
                dir = tmp * dir;
                up = tmp * up;
                // points -> vectors
                dir = normalize(dir - center);
                up = normalize(up - center);
            } break;
            default:
                throw "not implemented navigation srs type";
            }

            //vec3 physPos = convertor.navToPhys(vecFromUblas<vec3>(mapConfig->position.position));
            //center = avgRenderable;
            //dir = vec3(0, 0, -1);

            double dist = mapConfig->position.verticalExtent * 0.5 / tan(degToRad(mapConfig->position.verticalFov * 0.5));
            mat4 view = lookAt(center - dir * dist, center, up);
            mat4 proj = perspectiveMatrix(mapConfig->position.verticalFov, (double)width / (double)height, dist * 0.1, dist * 10);
            viewProj = proj * view;
        }

        void renderTick(uint32 width, uint32 height) override
        {
            mapConfig = map->resources->getMapConfig(map->mapConfigPath);
            if (!mapConfig || mapConfig->state != Resource::State::ready)
                return;

            shader = map->resources->getShader("data/shaders/a");
            if (!shader || shader->state != Resource::State::ready)
                return;
            shader->bind();

            if (!map->convertor)
            {
                map->convertor = std::shared_ptr<CsConvertor>(CsConvertor::create(
                    mapConfig->referenceFrame.model.physicalSrs,
                    mapConfig->referenceFrame.model.navigationSrs,
                    mapConfig->referenceFrame.model.publicSrs,
                    *mapConfig
                ));

                LOG(info3) << "position: " << mapConfig->position.position;
                LOG(info3) << "rotation: " << mapConfig->position.orientation;
            }

            updateCamera(width, height);

            binaryOrder = mapConfig->referenceFrame.metaBinaryOrder;
            for (auto &&surfConf : mapConfig->surfaces)
            {
                this->surfConf = &surfConf;
                targetLod = (this->surfConf->lodRange.min + this->surfConf->lodRange.max) / 2;
                metaUrlTemplate.parse(mapConfig->basePath + surfConf.urls3d->meta);
                meshUrlTemplate.parse(mapConfig->basePath + surfConf.urls3d->mesh);
                textureInternalUrlTemplate.parse(mapConfig->basePath + surfConf.urls3d->texture);
                traverse(TileId());
            }

            avgRenderable = sumRenderable / countRenderale;
            sumRenderable = vec3(0, 0, 0);
            countRenderale = 0;
        }

        void renderFinalize() override
        {
        }

        UrlTemplate metaUrlTemplate;
        UrlTemplate meshUrlTemplate;
        UrlTemplate textureInternalUrlTemplate;
        mat4 viewProj;
        MapImpl *map;
        MapConfig *mapConfig;
        SurfaceConfig *surfConf;
        GpuShader *shader;
        uint32 binaryOrder;

        vec3 lastRenderable;
        vec3 avgRenderable;
        vec3 sumRenderable;
        uint32 countRenderale;
        uint32 targetLod;
    };

    Renderer::Renderer()
    {}

    Renderer::~Renderer()
    {}

    Renderer *Renderer::create(MapImpl *map)
    {
        return new RendererImpl(map);
    }
}
