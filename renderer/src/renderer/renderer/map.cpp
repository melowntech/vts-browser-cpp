#include "../../dbglog/dbglog.hpp"

#include "map.h"
#include "mapConfig.h"
#include "cache.h"
#include "gpuManager.h"
#include "renderer.h"

#include <boost/thread/mutex.hpp>

namespace melown
{
    class MapImpl
    {
    public:
        boost::mutex *mut;
    };

    Map::Map(const std::string &mapConfigPath) : impl(nullptr), gpuManager(nullptr), renderer(nullptr), mapConfig(nullptr)
    {
        impl = new MapImpl;
        mapConfig = new MapConfig();
        gpuManager = GpuManager::create(this);
        renderer = Renderer::create(gpuManager);
    }

    Map::~Map()
    {
        delete impl;
    }

    void Map::dataInitialize(GpuContext *context, Fetcher *fetcher)
    {
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
