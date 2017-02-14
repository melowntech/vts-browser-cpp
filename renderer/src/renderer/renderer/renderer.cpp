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
            MetaTileTraverse() : map(nullptr), mapConfig(nullptr), surfConf(nullptr)
            {}

            std::shared_ptr<UrlTemplate> metaUrlTemplate;
            std::shared_ptr<UrlTemplate> meshUrlTemplate;
            Map *map;
            MapConfig *mapConfig;
            SurfaceConfig *surfConf;
            uint32 binaryOrder;

            void traverse(const TileId nodeId)
            {
                const TileId tileId(nodeId.lod, (nodeId.x >> binaryOrder) << binaryOrder, (nodeId.y >> binaryOrder) << binaryOrder);
                const MetaTile *tile = map->resources->getMetaTile((*metaUrlTemplate)(UrlTemplate::Vars(tileId)));
                if (!tile || !tile->ready)
                    return;
                const MetaNode *node = tile->get(nodeId, std::nothrow_t());
                if (!node)
                    return;

                LOG(info3) << nodeId.lod << " " << nodeId.x << " " << nodeId.y << " " << node->flags();

                if (node->ulChild() ) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 0, nodeId.y * 2 + 0));
                if (node->urChild() ) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 1, nodeId.y * 2 + 0));
                if (node->llChild() ) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 0, nodeId.y * 2 + 1));
                if (node->lrlChild()) traverse(TileId(nodeId.lod + 1, nodeId.x * 2 + 1, nodeId.y * 2 + 1));
            }

            void renderTick()
            {
                binaryOrder = mapConfig->referenceFrame.metaBinaryOrder;
                for (auto &&surfConf : mapConfig->surfaces)
                {
                    this->surfConf = &surfConf;
                    metaUrlTemplate = std::shared_ptr<UrlTemplate>(new UrlTemplate(mapConfig->basePath + surfConf.urls3d->meta));
                    meshUrlTemplate = std::shared_ptr<UrlTemplate>(new UrlTemplate(mapConfig->basePath + surfConf.urls3d->mesh));
                    traverse(TileId());
                }
            }
        };

        void renderTick() override
        {
            LOG(info4) << "RENDER FRAME START";

            MetaTileTraverse mtt;
            mtt.map = map;
            mtt.mapConfig = map->resources->getMapConfig(map->mapConfigPath);
            if (mtt.mapConfig && mtt.mapConfig->ready)
                mtt.renderTick();

            LOG(info4) << "RENDER FRAME DONE";
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
