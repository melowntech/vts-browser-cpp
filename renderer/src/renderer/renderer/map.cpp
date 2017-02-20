#include "../../dbglog/dbglog.hpp"

#include <renderer/map.h>

#include "map.h"
#include "renderer.h"
#include "resourceManager.h"

namespace melown
{
    MapFoundation::MapFoundation(const std::string &mapConfigPath)
    {
        impl = std::shared_ptr<MapImpl>(new MapImpl(mapConfigPath));
    }

    MapFoundation::~MapFoundation()
    {}

    void MapFoundation::dataInitialize(GpuContext *context, Fetcher *fetcher)
    {
        dbglog::thread_id("data");
        impl->resources->dataInitialize(context, fetcher);
    }

    bool MapFoundation::dataTick()
    {
        return impl->resources->dataTick();
    }

    void MapFoundation::dataFinalize()
    {
        impl->resources->dataFinalize();
    }

    void MapFoundation::renderInitialize(GpuContext *context)
    {
        dbglog::thread_id("render");
        impl->renderer->renderInitialize();
        impl->resources->renderInitialize(context);
    }

    void MapFoundation::renderTick(uint32 width, uint32 height)
    {
        impl->renderer->renderTick(width, height);
        impl->resources->renderTick();
    }

    void MapFoundation::renderFinalize()
    {
        impl->renderer->renderFinalize();
        impl->resources->renderFinalize();
    }

    MapImpl::MapImpl(const std::string &mapConfigPath) : mapConfigPath(mapConfigPath)
    {
        resources = std::shared_ptr<ResourceManager>(ResourceManager::create(this));
        renderer = std::shared_ptr<Renderer>(Renderer::create(this));
    }
}
