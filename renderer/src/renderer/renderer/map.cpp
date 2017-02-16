#include "../../dbglog/dbglog.hpp"

#include "map.h"
#include "cache.h"
#include "renderer.h"
#include "resourceManager.h"

namespace melown
{
    class LogInitializer
    {
    public:
        LogInitializer()
        {
            dbglog::log_pid(false);
        }
    } logInitializer;

    Map::Map(const std::string &mapConfigPath) : resources(nullptr), renderer(nullptr), mapConfigPath(mapConfigPath)
    {
        resources = ResourceManager::create(this);
        renderer = Renderer::create(this);
    }

    Map::~Map()
    {
        delete renderer; renderer = nullptr;
        delete resources; resources = nullptr;
    }

    void Map::dataInitialize(GpuContext *context, Fetcher *fetcher)
    {
        dbglog::thread_id("data");
        resources->dataInitialize(context, fetcher);
    }

    bool Map::dataTick()
    {
        return resources->dataTick();
    }

    void Map::dataFinalize()
    {
        resources->dataFinalize();
    }

    void Map::renderInitialize(GpuContext *context)
    {
        dbglog::thread_id("render");
        renderer->renderInitialize();
        resources->renderInitialize(context);
    }

    void Map::renderTick(uint32 width, uint32 height)
    {
        renderer->renderTick(width, height);
        resources->renderTick();
    }

    void Map::renderFinalize()
    {
        renderer->renderFinalize();
        resources->renderFinalize();
    }
}
