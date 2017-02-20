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

            if (nodeId.lod <= 14)
            { // traverse children
                if (node->ulChild() ) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 0, nodeId.y * 2 + 0));
                if (node->urChild() ) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 1, nodeId.y * 2 + 0));
                if (node->llChild() ) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 0, nodeId.y * 2 + 1));
                if (node->lrlChild()) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 1, nodeId.y * 2 + 1));
            }

            if (node->geometry() && nodeId.lod == 15)
            {
                MeshAggregate *meshAgg = map->resources->getMeshAggregate(meshUrlTemplate(UrlTemplate::Vars(nodeId)));
                if (meshAgg && meshAgg->state == Resource::State::ready)
                {
                    for (uint32 i = 0, e = meshAgg->submeshes.size(); i != e; i++)
                    {
                        GpuMeshRenderable *mesh = meshAgg->submeshes[i].get();
                        //GpuTexture *texture = map->resources->getTexture(textureInternalUrlTemplate(UrlTemplate::Vars(nodeId).addSubmesh(i)));
                        //if (texture && texture->ready)
                        //{
                        //    texture->bind();
                            mat4f mvp = viewProj.cast<float>();
                            shader->uniformMat4(0, mvp.data());
                            mesh->draw();
                        //}
                    }
                }
            }
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

            { // orbiting camera
                mapConfig->position.orientation(0) += 0.1;
                mapConfig->position.orientation(1) = -60;
            }

            convertor.configure(
                        mapConfig->referenceFrame.model.physicalSrs,
                        mapConfig->referenceFrame.model.navigationSrs,
                        mapConfig->referenceFrame.model.publicSrs,
                        *mapConfig
            );

            { // camera
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
                    center = convertor.navToPhys(center);
                    dir = convertor.navToPhys(dir);
                    up = convertor.navToPhys(up);
                    // subtract center (make them vectors again)
                    dir -= center;
                    up -= center;
                    dir = normalize(dir);
                    up = normalize(up);
                } break;
                default:
                    throw "not implemented navigation srs type";
                }

                float dist = mapConfig->position.verticalExtent * 0.5f / tan(degToRad(mapConfig->position.verticalFov * 0.5));

                mat4 view = lookAt(center - dir * dist, center, up);
                mat4 proj = perspectiveMatrix(mapConfig->position.verticalFov, (double)width / (double)height, dist * 0.1, dist * 10);

                viewProj = proj * view;
            }

            binaryOrder = mapConfig->referenceFrame.metaBinaryOrder;
            for (auto &&surfConf : mapConfig->surfaces)
            {
                this->surfConf = &surfConf;
                metaUrlTemplate.parse(mapConfig->basePath + surfConf.urls3d->meta);
                meshUrlTemplate.parse(mapConfig->basePath + surfConf.urls3d->mesh);
                textureInternalUrlTemplate.parse(mapConfig->basePath + surfConf.urls3d->texture);
                traverse(TileId());
            }
        }

        void renderFinalize() override
        {
        }

        CsConvertor convertor;
        UrlTemplate metaUrlTemplate;
        UrlTemplate meshUrlTemplate;
        UrlTemplate textureInternalUrlTemplate;
        mat4 viewProj;
        MapImpl *map;
        MapConfig *mapConfig;
        SurfaceConfig *surfConf;
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
}
