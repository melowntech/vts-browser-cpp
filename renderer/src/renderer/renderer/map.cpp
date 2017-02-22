#include "../../dbglog/dbglog.hpp"

#include <renderer/map.h>

#include "map.h"
#include "renderer.h"
#include "resourceManager.h"
#include "mapConfig.h"
#include "csConvertor.h"

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
        if (mapConfig && mapConfig->state == Resource::State::ready && impl->convertor)
        {
            vadstena::registry::Position &pos = mapConfig->position;
            mat3 rot = upperLeftSubMatrix(rotationMatrix(2, degToRad(pos.orientation(0))));
            double vertFactor = 1; //abs(sin(pos.orientation(1))) * 0.5 + 0.5;
            vec3 move = vec3(-value[0], value[1] * vertFactor, 0);
            move = rot * move * (pos.verticalExtent / 800);
            switch (mapConfig->srs.get(mapConfig->referenceFrame.model.navigationSrs).type)
            {
            case vadstena::registry::Srs::Type::projected:
                break; // do nothing
            case vadstena::registry::Srs::Type::geographic:
                // todo
                break;
            default:
                throw "not implemented navigation srs type";
            }
            pos.position += vecToUblas<math::Point3>(move);
            pos.verticalExtent *= pow(1.001, -value[2]);
        }
    }

    void MapFoundation::rotate(const double value[3])
    {
        MapConfig *mapConfig = impl->resources->getMapConfig(impl->mapConfigPath);
        if (mapConfig && mapConfig->state == Resource::State::ready)
        {
            vadstena::registry::Position &pos = mapConfig->position;
            pos.orientation += vecToUblas<math::Point3>(vec3(value[0] * -0.2, value[1] * -0.1, 0));
        }
    }

    MapImpl::MapImpl(const std::string &mapConfigPath) : mapConfigPath(mapConfigPath)
    {
        resources = std::shared_ptr<ResourceManager>(ResourceManager::create(this));
        renderer = std::shared_ptr<Renderer>(Renderer::create(this));
    }
}
