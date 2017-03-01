#include <dbglog/dbglog.hpp>

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

namespace {
double modulo(double a, double m)
{
    int b = (int)(a / m);
    return a - m * b;
}
}

void MapFoundation::pan(const double value[3])
{
    MapConfig *mapConfig = impl->resources->getMapConfig(impl->mapConfigPath);
    if (!mapConfig || mapConfig->state != Resource::State::ready
            || !impl->convertor)
        return;

    vtslibs::registry::Position &pos = mapConfig->position;
    switch (mapConfig->srs.get
            (mapConfig->referenceFrame.model.navigationSrs).type)
    {
    case vtslibs::registry::Srs::Type::projected:
    {
        mat3 rot = upperLeftSubMatrix
                (rotationMatrix(2, degToRad(pos.orientation(0))));
        vec3 move = vec3(-value[0], value[1], 0);
        move = rot * move * (pos.verticalExtent / 800);
        pos.position += vecToUblas<math::Point3>(move);
    } break;
    case vtslibs::registry::Srs::Type::geographic:
    {
        mat3 rot = upperLeftSubMatrix
                (rotationMatrix(2, degToRad(-pos.orientation(0))));
        vec3 move = vec3(-value[0], value[1], 0);
        move = rot * move * (pos.verticalExtent / 800);
        vec3 p = vecFromUblas<vec3>(pos.position);
        p = impl->convertor->navGeodesicDirect(p, 90, move(0));
        p = impl->convertor->navGeodesicDirect(p, 0, move(1));
        pos.position = vecToUblas<math::Point3>(p);
        pos.position(0) = modulo(pos.position(0) + 180, 360) - 180;
        // todo - temporarily clamp
        pos.position(1) = std::min(std::max(pos.position(1), -80.0), 80.0);
    } break;
    default:
        throw "not implemented navigation srs type";
    }
    pos.verticalExtent *= pow(1.001, -value[2]);

    //LOG(info3) << "position: " << mapConfig->position.position;
    //LOG(info3) << "rotation: " << mapConfig->position.orientation;
}

void MapFoundation::rotate(const double value[3])
{
    MapConfig *mapConfig = impl->resources->getMapConfig(impl->mapConfigPath);
    if (!mapConfig || mapConfig->state != Resource::State::ready)
        return;

    vtslibs::registry::Position &pos = mapConfig->position;
    vec3 rot(value[0] * -0.2, value[1] * -0.1, 0);
    switch (mapConfig->srs.get
            (mapConfig->referenceFrame.model.navigationSrs).type)
    {
    case vtslibs::registry::Srs::Type::projected:
        break; // do nothing
    case vtslibs::registry::Srs::Type::geographic:
        rot(0) *= -1;
        break;
    default:
        throw "not implemented navigation srs type";
    }
    pos.orientation += vecToUblas<math::Point3>(rot);

    //LOG(info3) << "position: " << mapConfig->position.position;
    //LOG(info3) << "rotation: " << mapConfig->position.orientation;
}

MapImpl::MapImpl(const std::string &mapConfigPath)
    : mapConfigPath(mapConfigPath)
{
    resources = std::shared_ptr<ResourceManager>(ResourceManager::create(this));
    renderer = std::shared_ptr<Renderer>(Renderer::create(this));
}

} // namespace melown
