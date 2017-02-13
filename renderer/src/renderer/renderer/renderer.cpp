#include "renderer.h"
#include "gpuManager.h"
#include "gpuResources.h"

namespace melown
{
    class RendererImpl : public Renderer
    {
    public:
        RendererImpl(GpuManager *gpuManager)
        {
            this->gpuManager = gpuManager;
        }

        void renderInitialize() override
        {
        }

        void renderTick() override
        {

            // test
            bool ready = gpuManager->getTexture("http://m2.mapserver.mapy.cz/ophoto/13_07f58000_081e0000")->ready;

        }

        void renderFinalize() override
        {
        }
    };

    Renderer::Renderer() : gpuManager(nullptr)
    {}

    Renderer::~Renderer()
    {}

    Renderer *Renderer::create(GpuManager *gpuManager)
    {
        return new RendererImpl(gpuManager);
    }
}
