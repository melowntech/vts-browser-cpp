#include <vts/map.hpp>

#include "map.hpp"

namespace vts
{

Point::Point() : x(0), y(0), z(0)
{}

Point::Point(double x, double y, double z) : x(x), y(y), z(z)
{}

namespace
{

const vec3 vecFromPoint(const Point &p)
{
    vec3 v;
    for (uint32 i = 0; i < 3; i++)
        v(i) = p.data[i];
    return v;
}

const Point vecToPoint(const vec3 &p)
{
    Point v;
    for (uint32 i = 0; i < 3; i++)
        v.data[i] = p(i);
    return v;
}

const std::string srsConvert(MapConfig *config, Srs srs)
{
    switch(srs)
    {
    case Srs::Navigation:
        return config->referenceFrame.model.navigationSrs;
    case Srs::Physical:
        return config->referenceFrame.model.physicalSrs;
    case Srs::Public:
        return config->referenceFrame.model.publicSrs;
    default:
        throw std::invalid_argument("Invalid srs");
    }
}

} // namespace

MapFoundationOptions::MapFoundationOptions() : keepInvalidUrls(false)
{}

MapFoundation::MapFoundation(const MapFoundationOptions &options)
{
    impl = std::shared_ptr<MapImpl>(new MapImpl(this, options));
}

MapFoundation::~MapFoundation()
{}

void MapFoundation::dataInitialize(Fetcher *fetcher)
{
    dbglog::thread_id("data");
    impl->dataInitialize(fetcher);
}

bool MapFoundation::dataTick()
{
    return impl->dataTick();
}

void MapFoundation::dataFinalize()
{
    impl->dataFinalize();
}

void MapFoundation::renderInitialize()
{
    dbglog::thread_id("render");
    impl->dataRenderInitialize();
    impl->renderInitialize();
}

void MapFoundation::renderTick(uint32 width, uint32 height)
{
    impl->statistics.resetFrame();
    impl->dataRenderTick();
    impl->renderTick(width, height);
}

void MapFoundation::renderFinalize()
{
    impl->dataRenderFinalize();
    impl->renderFinalize();
}

void MapFoundation::setMapConfig(const std::string &mapConfigPath)
{
    impl->setMapConfig(mapConfigPath);
}

void MapFoundation::pan(const Point &value)
{
    impl->pan(vecFromPoint(value));
}

void MapFoundation::rotate(const Point &value)
{
    impl->rotate(vecFromPoint(value)); 
}

MapStatistics &MapFoundation::statistics()
{
    return impl->statistics;
}

MapOptions &MapFoundation::options()
{
    return impl->options;
}

DrawBatch &MapFoundation::drawBatch()
{
    return impl->draws;
}

void MapFoundation::setPositionPoint(const Point &point)
{
    if (!impl->mapConfig || !*impl->mapConfig)
        return;
    impl->mapConfig->position.position
            = vecToUblas<math::Point3>(vecFromPoint(point));
}

const Point MapFoundation::getPositionPoint()
{
    if (!impl->mapConfig || !*impl->mapConfig)
        return vecToPoint(vec3());
    return vecToPoint(vecFromUblas<vec3>(impl->mapConfig->position.position));
}

void MapFoundation::setPositionRotation(const Point &point)
{
    if (!impl->mapConfig || !*impl->mapConfig)
        return;
    impl->mapConfig->position.orientation
            = vecToUblas<math::Point3>(vecFromPoint(point));
}

const Point MapFoundation::getPositionRotation()
{
    if (!impl->mapConfig || !*impl->mapConfig)
        return vecToPoint(vec3());
    return vecToPoint(vecFromUblas<vec3>(
                          impl->mapConfig->position.orientation));
}

void MapFoundation::setPositionViewExtent(double viewExtent)
{
    if (!impl->mapConfig || !*impl->mapConfig)
        return;
    impl->mapConfig->position.verticalExtent = viewExtent;
}

double MapFoundation::getPositionViewExtent()
{
    if (!impl->mapConfig || !*impl->mapConfig)
        return 0;
    return impl->mapConfig->position.verticalExtent;
}

void MapFoundation::setPositionFov(double fov)
{
    if (!impl->mapConfig || !*impl->mapConfig)
        return;
    impl->mapConfig->position.verticalFov = fov;
}

double MapFoundation::getPositionFov()
{
    if (!impl->mapConfig || !*impl->mapConfig)
        return 0;
    return impl->mapConfig->position.verticalFov;
}

const Point MapFoundation::convert(const Point &point, Srs from, Srs to)
{
    if (!impl->mapConfig || !*impl->mapConfig)
        return vecToPoint(vec3());
    return vecToPoint(impl->mapConfig->convertor->convert(vecFromPoint(point),
                srsConvert(impl->mapConfig.get(), from),
                srsConvert(impl->mapConfig.get(), to)));
}

MapImpl::MapImpl(MapFoundation *mapFoundation,
                 const MapFoundationOptions &options) :
    resources(options.cachePath, options.keepInvalidUrls),
    mapFoundation(mapFoundation), initialized(false)
{}

} // namespace vts
