#include "map.h"
#include "renderer.h"
#include "gpuManager.h"
#include "gpuResources.h"

namespace melown
{
    class RendererImpl : public Renderer
    {
    public:
        RendererImpl(Map *map) : map(map)
        {}

        void renderInitialize() override
        {
        }

        void renderTick() override
        {
            if (!map->mapConfig)
                return;

            // test
            bool ready = map->gpuManager->getTexture("http://m2.mapserver.mapy.cz/ophoto/13_07f58000_081e0000")->ready;
            bool ready2 = map->gpuManager->getMeshAggregate("https://david.test.mlwn.se/maps/j8.pp/tilesets/jenstejn-hf/12-2036-2017.bin?1")->ready;

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
