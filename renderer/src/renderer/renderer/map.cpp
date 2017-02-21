#include "../../dbglog/dbglog.hpp"

#include <renderer/map.h>

#include "map.h"
#include "renderer.h"
#include "resourceManager.h"
#include "mapConfig.h"

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

    void MapFoundation::pan(const double value[3])
    {
        MapConfig *mapConfig = impl->resources->getMapConfig(impl->mapConfigPath);
        if (mapConfig && mapConfig->state == Resource::State::ready)
        {
            vadstena::registry::Position &pos = mapConfig->position;
            for (uint32 i = 0; i < 3; i++)
                pos.position(i) += value[i];
        }
    }

    void MapFoundation::rotate(const double value[3])
    {
        MapConfig *mapConfig = impl->resources->getMapConfig(impl->mapConfigPath);
        if (mapConfig && mapConfig->state == Resource::State::ready)
        {
            vadstena::registry::Position &pos = mapConfig->position;
            for (uint32 i = 0; i < 3; i++)
                pos.orientation(i) += value[i];
        }
    }

    MapImpl::MapImpl(const std::string &mapConfigPath) : mapConfigPath(mapConfigPath)
    {
        resources = std::shared_ptr<ResourceManager>(ResourceManager::create(this));
        renderer = std::shared_ptr<Renderer>(Renderer::create(this));
    }
}
