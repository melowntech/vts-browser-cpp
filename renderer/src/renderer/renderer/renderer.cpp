#include "map.h"
#include "renderer.h"
#include "resourceManager.h"
#include "mapResources.h"
#include "gpuResources.h"

#include "../../vts-libs/vts/urltemplate.hpp"
#include "../../dbglog/dbglog.hpp"

namespace melown
{
    using namespace vadstena::vts;
    using namespace vadstena::registry;

    class RendererImpl : public Renderer
    {
    public:
        RendererImpl(Map *map) : map(map)
        {}

        void renderInitialize() override
        {
        }

        struct MetaTileTraverse
        {
            MetaTileTraverse() : map(nullptr), mapConfig(nullptr), surfConf(nullptr), shader(nullptr)
            {}

            UrlTemplate metaUrlTemplate;
            UrlTemplate meshUrlTemplate;
            UrlTemplate textureInternalUrlTemplate;
            Map *map;
            MapConfig *mapConfig;
            SurfaceConfig *surfConf;
            uint32 binaryOrder;
            GpuShader *shader;

            mat4 viewProj;

            void traverse(const TileId nodeId)
            {
                const TileId tileId(nodeId.lod, (nodeId.x >> binaryOrder) << binaryOrder, (nodeId.y >> binaryOrder) << binaryOrder);
                const MetaTile *tile = map->resources->getMetaTile(metaUrlTemplate(UrlTemplate::Vars(tileId)));
                if (!tile || !tile->ready)
                    return;
                const MetaNode *node = tile->get(nodeId, std::nothrow_t());
                if (!node)
                    return;

                { // traverse children
                    if (node->ulChild() ) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 0, nodeId.y * 2 + 0));
                    if (node->urChild() ) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 1, nodeId.y * 2 + 0));
                    if (node->llChild() ) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 0, nodeId.y * 2 + 1));
                    if (node->lrlChild()) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 1, nodeId.y * 2 + 1));
                }

                if (node->geometry())
                {
                    MeshAggregate *meshAgg = map->resources->getMeshAggregate(meshUrlTemplate(UrlTemplate::Vars(nodeId)));
                    if (meshAgg && meshAgg->ready)
                    {
                        for (uint32 i = 0, e = meshAgg->submeshes.size(); i != e; i++)
                        {
                            GpuMeshRenderable *mesh = meshAgg->submeshes[i].get();
                            GpuTexture *texture = map->resources->getTexture(textureInternalUrlTemplate(UrlTemplate::Vars(nodeId).addSubmesh(i)));
                            if (texture && texture->ready)
                            {
                                texture->bind();
                                mat4 mvp = viewProj;
                                shader->uniform(0, mvp);
                                mesh->draw();
                            }
                        }
                    }
                }
            }

            void renderTick(uint32 width, uint32 height)
            {
                mapConfig = map->resources->getMapConfig(map->mapConfigPath);
                if (!mapConfig || !mapConfig->ready)
                    return;

                shader = map->resources->getShader("data/shaders/a");
                if (!shader || !shader->ready)
                    return;
                shader->bind();

                static float angle = 0;
                angle += 0.02;

                vec3 target = vec3(472800, 5.55472e+06, 232.794);
                mat4 view = lookAt(target + vec3(sin(angle), cos(angle), 0) * 5000 + vec3(0, 0, 5000), target, vec3(0, 0, 1));
                mat4 proj = perspectiveMatrix(60, (float)width / (float)height, 100, 10000);

                viewProj = proj * view;

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
        };

        void renderTick(uint32 width, uint32 height) override
        {
            MetaTileTraverse mtt;
            mtt.map = map;
            mtt.renderTick(width, height);
        }

        void renderFinalize() override
        {
        }

        Map *map;
    };

    Renderer::Renderer()
    {}

    Renderer::~Renderer()
    {}

    Renderer *Renderer::create(Map *map)
    {
        return new RendererImpl(map);
    }
}
