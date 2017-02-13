#include "../../dbglog/dbglog.hpp"

#include "map.h"
#include "mapConfig.h"
#include "cache.h"
#include "gpuManager.h"
#include "renderer.h"

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

    Map::Map(const std::string &mapConfigPath) : gpuManager(nullptr), renderer(nullptr), mapConfig(nullptr), mapConfigPath(mapConfigPath)
    {
        gpuManager = GpuManager::create(this);
        renderer = Renderer::create(this);
    }

    Map::~Map()
    {
        delete renderer; renderer = nullptr;
        delete gpuManager; gpuManager = nullptr;
        delete mapConfig; mapConfig = nullptr;
    }

    void Map::dataInitialize(GpuContext *context, Fetcher *fetcher)
    {
        dbglog::thread_id("data");
        gpuManager->dataInitialize(context, fetcher);
    }

    bool Map::dataTick()
    {
        return gpuManager->dataTick();
    }

    void Map::dataFinalize()
    {
        gpuManager->dataFinalize();
    }

    void Map::renderInitialize(GpuContext *context)
    {
        dbglog::thread_id("render");
        renderer->renderInitialize();
        gpuManager->renderInitialize(context);
    }

    void Map::renderTick()
    {
        renderer->renderTick();
        gpuManager->renderTick();
    }

    void Map::renderFinalize()
    {
        renderer->renderFinalize();
        gpuManager->renderFinalize();
    }
}
