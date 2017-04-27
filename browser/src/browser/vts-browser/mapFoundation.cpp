#include <vts/map.hpp>
#include <vts/view.hpp>
#include <vts-libs/registry/json.hpp>
#include <boost/algorithm/string.hpp>

#include "map.hpp"

namespace vts
{

namespace
{

inline const vec3 vecFromPoint(const Point &in)
{
    vec3 r;
    vecFromPoint(in, r);
    return r;
}

inline const Point vecToPoint(const vec3 &in)
{
    Point r;
    vecToPoint(in, r);
    return r;
}

inline const std::string srsConvert(MapConfig *config, Srs srs)
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

void getMapViewBoundLayers(
        const vtslibs::registry::View::BoundLayerParams::list &in,
        MapView::BoundLayerInfo::Map &out)
{
    out.clear();
    for (auto &&it : in)
    {
        MapView::BoundLayerInfo b(it.id);
        b.alpha = it.alpha ? *it.alpha : 1;
        out.push_back(b);
    }
}

void setMapViewBoundLayers(
        const MapView::BoundLayerInfo::Map &in,
        vtslibs::registry::View::BoundLayerParams::list &out
        )
{
    out.clear();
    for (auto &&it : in)
    {
        vtslibs::registry::View::BoundLayerParams b;
        b.id = it.id;
        if (it.alpha < 1 - 1e-7)
            b.alpha = it.alpha;
        out.push_back(b);
    }
}

const MapView getMapView(const vtslibs::registry::View &view)
{
    MapView value;
    value.description = view.description ? *view.description : "";
    for (auto &&it : view.surfaces)
    {
        MapView::SurfaceInfo &s = value.surfaces[it.first];
        getMapViewBoundLayers(it.second, s.boundLayers);
    }
    for (auto &&it : view.freeLayers)
    {
        MapView::FreeLayerInfo &f = value.freeLayers[it.first];
        f.style = it.second.style ? *it.second.style : "";
        getMapViewBoundLayers(it.second.boundLayers, f.boundLayers);
    }
    return std::move(value);
}

const vtslibs::registry::View setMapView(const MapView &value)
{
    vtslibs::registry::View view;
    if (!value.description.empty())
        view.description = value.description;
    for (auto &&it : value.surfaces)
    {
        vtslibs::registry::View::BoundLayerParams::list &b
                = view.surfaces[it.first];
        setMapViewBoundLayers(it.second.boundLayers, b);
    }
    for (auto &&it : value.freeLayers)
    {
        vtslibs::registry::View::FreeLayerParams &f
                = view.freeLayers[it.first];
        if (!it.second.style.empty())
            f.style = it.second.style;
        setMapViewBoundLayers(it.second.boundLayers, f.boundLayers);
    }
    return std::move(view);
}

} // namespace

MapView::BoundLayerInfo::BoundLayerInfo() : alpha(1)
{}

MapView::BoundLayerInfo::BoundLayerInfo(const std::string &id) :
    id(id), alpha(1)
{}

Point::Point() : x(0), y(0), z(0)
{}

Point::Point(double x, double y, double z) : x(x), y(y), z(z)
{}

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

void MapFoundation::setMapConfigPath(const std::string &mapConfigPath)
{
    impl->setMapConfigPath(mapConfigPath);
}

const std::string MapFoundation::getMapConfigPath() const
{
    return impl->mapConfigPath;
}

void MapFoundation::purgeTraverseCache(bool hard)
{
    if (hard)
        impl->purgeHard();
    else
        impl->purgeSoft();
}

bool MapFoundation::isMapConfigReady() const
{
    return impl->mapConfig && *impl->mapConfig;
}

bool MapFoundation::isMapRenderComplete() const
{
    if (!isMapConfigReady())
        return false;
    return !impl->draws.draws.empty()
            && impl->statistics.currentNodeUpdates == 0;
}

double MapFoundation::getMapRenderProgress() const
{
    if (!isMapConfigReady())
        return 0;
    if (isMapRenderComplete())
        return 1;
    return 0.5; // todo some better heuristic
}

void MapFoundation::pan(const Point &value)
{
    if (!isMapConfigReady())
        return;
    impl->pan(vecFromPoint(value));
}

void MapFoundation::rotate(const Point &value)
{
    if (!isMapConfigReady())
        return;
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

void MapFoundation::setPositionSubjective(bool subjective, bool convert)
{
    if (!isMapConfigReady() || subjective == getPositionSubjective())
        return;
    if (convert)
        impl->convertPositionSubjObj();
    impl->mapConfig->position.type = subjective
            ? vtslibs::registry::Position::Type::subjective
            : vtslibs::registry::Position::Type::objective;
}

bool MapFoundation::getPositionSubjective() const
{
    if (!isMapConfigReady())
        return false;
    return impl->mapConfig->position.type
            == vtslibs::registry::Position::Type::subjective;
}

void MapFoundation::setPositionPoint(const Point &point)
{
    if (!isMapConfigReady())
        return;
    impl->mapConfig->position.position
            = vecToUblas<math::Point3>(vecFromPoint(point));
    impl->navigation.inertiaMotion = vec3(0,0,0);
}

const Point MapFoundation::getPositionPoint() const
{
    if (!isMapConfigReady())
        return vecToPoint(vec3(0,0,0));
    return vecToPoint(vecFromUblas<vec3>(impl->mapConfig->position.position));
}

void MapFoundation::setPositionRotation(const Point &point)
{
    if (!impl->mapConfig || !*impl->mapConfig)
        return;
    impl->mapConfig->position.orientation
            = vecToUblas<math::Point3>(vecFromPoint(point));
    impl->navigation.inertiaRotation = vec3(0,0,0);
}

const Point MapFoundation::getPositionRotation() const
{
    if (!isMapConfigReady())
        return vecToPoint(vec3(0,0,0));
    return vecToPoint(vecFromUblas<vec3>(
                          impl->mapConfig->position.orientation));
}

void MapFoundation::setPositionViewExtent(double viewExtent)
{
    if (!isMapConfigReady())
        return;
    impl->mapConfig->position.verticalExtent = viewExtent;
    impl->navigation.inertiaViewExtent = 0;
}

double MapFoundation::getPositionViewExtent() const
{
    if (!isMapConfigReady())
        return 0;
    return impl->mapConfig->position.verticalExtent;
}

void MapFoundation::setPositionFov(double fov)
{
    if (!isMapConfigReady())
        return;
    impl->mapConfig->position.verticalFov = fov;
}

double MapFoundation::getPositionFov() const
{
    if (!isMapConfigReady())
        return 0;
    return impl->mapConfig->position.verticalFov;
}

const std::string MapFoundation::getPositionJson() const
{
    if (!isMapConfigReady())
        return "";
    return Json::FastWriter().write(
                vtslibs::registry::asJson(impl->mapConfig->position));
}

void MapFoundation::setPositionJson(const std::string &position)
{
    if (!isMapConfigReady())
        return;
    Json::Value val;
    if (!Json::Reader().parse(position, val))
        throw std::runtime_error("invalid position json");
    impl->mapConfig->position = vtslibs::registry::positionFromJson(val);
}

void MapFoundation::resetPositionAltitude()
{
    impl->resetPositionAltitude(0);
}

const Point MapFoundation::convert(const Point &point, Srs from, Srs to) const
{
    if (!isMapConfigReady())
        return vecToPoint(vec3(0,0,0));
    return vecToPoint(impl->mapConfig->convertor->convert(vecFromPoint(point),
                srsConvert(impl->mapConfig.get(), from),
                srsConvert(impl->mapConfig.get(), to)));
}

const std::vector<std::string> MapFoundation::getResourceSurfaces() const
{
    if (!isMapConfigReady())
        return {};
    std::vector<std::string> names;
    names.reserve(impl->mapConfig->surfaces.size());
    for (auto &&it : impl->mapConfig->surfaces)
        names.push_back(it.id);
    return std::move(names);
}

const std::vector<std::string> MapFoundation::getResourceBoundLayers() const
{
    if (!isMapConfigReady())
        return {};
    std::vector<std::string> names;
    for (auto &&it : impl->mapConfig->boundLayers)
        names.push_back(it.id);
    return std::move(names);
}

const std::vector<std::string> MapFoundation::getResourceFreeLayers() const
{
    if (!isMapConfigReady())
        return {};
    std::vector<std::string> names;
    for (auto &&it : impl->mapConfig->freeLayers)
        names.push_back(it.first);
    return std::move(names);
}

const std::vector<std::string> MapFoundation::getViewNames() const
{
    if (!isMapConfigReady())
        return {};
    std::vector<std::string> names;
    names.reserve(impl->mapConfig->namedViews.size());
    for (auto &&it : impl->mapConfig->namedViews)
        names.push_back(it.first);
    return std::move(names);
}

const std::string MapFoundation::getViewCurrent() const
{
    if (!isMapConfigReady())
        return "";
    return impl->mapConfigView;
}

void MapFoundation::setViewCurrent(const std::string &name)
{
    if (!isMapConfigReady())
        return;
    auto it = impl->mapConfig->namedViews.find(name);
    if (it == impl->mapConfig->namedViews.end())
        throw std::runtime_error("invalid mapconfig view name");
    impl->mapConfig->view = it->second;
    impl->purgeSoft();
    impl->mapConfigView = name;
}

void MapFoundation::getViewData(const std::string &name, MapView &view) const
{
    if (!isMapConfigReady())
        return;
    if (name == "")
    {
        view = getMapView(impl->mapConfig->view);
        return;
    }
    auto it = impl->mapConfig->namedViews.find(name);
    if (it == impl->mapConfig->namedViews.end())
        throw std::runtime_error("invalid mapconfig view name");
    view = getMapView(it->second);
}

void MapFoundation::setViewData(const std::string &name, const MapView &view)
{
    if (!isMapConfigReady())
        return;
    if (name == "")
    {
        impl->mapConfig->view = setMapView(view);
        impl->purgeSoft();
    }
    else
        impl->mapConfig->namedViews[name] = setMapView(view);
}

const std::string MapFoundation::getViewJson(const std::string &name) const
{
    if (!isMapConfigReady())
        return "";
    if (name == "")
        return Json::FastWriter().write(vtslibs::registry::asJson(
                impl->mapConfig->view, impl->mapConfig->boundLayers));
    auto it = impl->mapConfig->namedViews.find(name);
    if (it == impl->mapConfig->namedViews.end())
        throw std::runtime_error("invalid mapconfig view name");
    return Json::FastWriter().write(vtslibs::registry::asJson(it->second,
                        impl->mapConfig->boundLayers));
}

void MapFoundation::setViewJson(const std::string &name,
                                const std::string &view)
{
    if (!isMapConfigReady())
        return;
    Json::Value val;
    if (!Json::Reader().parse(view, val))
        throw std::runtime_error("invalid view json");
    if (name == "")
    {
        impl->mapConfig->view = vtslibs::registry::viewFromJson(val);
        impl->purgeSoft();
    }
    else
        impl->mapConfig->namedViews[name]
                = vtslibs::registry::viewFromJson(val);
}

void MapFoundation::printDebugInfo()
{
    impl->printDebugInfo();
}

MapImpl::MapImpl(MapFoundation *mapFoundation,
                 const MapFoundationOptions &options) :
    resources(options.cachePath, options.keepInvalidUrls),
    mapFoundation(mapFoundation), initialized(false)
{}

void MapImpl::printDebugInfo()
{
    LOG(info3) << "-----DEBUG INFO-----";
    LOG(info3) << "mapconfig path: " << mapConfigPath;
    if (!mapConfig)
    {
        LOG(info3) << "missing mapconfig";
        return;
    }
    if (!*mapConfig)
    {
        LOG(info3) << "mapconfig not ready";
        return;
    }
    LOG(info3) << "Position: " << mapFoundation->getPositionJson();
    LOG(info3) << "Named views: " << boost::join(mapFoundation->getViewNames(),
                                                 ", ");
    LOG(info3) << "Current view name: " << mapFoundation->getViewCurrent();
    LOG(info3) << "Current view data: " << mapFoundation->getViewJson("");
    mapConfig->printSurfaceStack();
}

} // namespace vts
