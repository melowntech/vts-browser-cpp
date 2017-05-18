#include <vts-libs/registry/json.hpp>
#include <boost/algorithm/string.hpp>

#include "include/vts-browser/map.hpp"
#include "include/vts-browser/view.hpp"
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
        LOGTHROW(fatal, std::invalid_argument) << "Invalid srs";
    }
    throw;
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

Point::Point()
{
    data[0] = data[1] = data[2] = 0;
}

Point::Point(double x, double y, double z)
{
    data[0] = x;
    data[1] = y;
    data[2] = z;
}

MapCreateOptions::MapCreateOptions() : disableCache(false)
{}

Map::Map(const MapCreateOptions &options)
{
    LOG(info4) << "Creating map";
    impl = std::shared_ptr<MapImpl>(new MapImpl(this, options));
}

Map::~Map()
{
    LOG(info4) << "Destroying map";
}

void Map::dataInitialize(Fetcher *fetcher)
{
    dbglog::thread_id("data");
    impl->resourceDataInitialize(fetcher);
}

bool Map::dataTick()
{
    return impl->resourceDataTick();
}

void Map::dataFinalize()
{
    impl->resourceDataFinalize();
}

void Map::renderInitialize()
{
    dbglog::thread_id("render");
    impl->resourceRenderInitialize();
    impl->renderInitialize();
}

void Map::renderTick(uint32 width, uint32 height)
{
    impl->statistics.resetFrame();
    impl->resourceRenderTick();
    impl->renderTick(width, height);
}

void Map::renderFinalize()
{
    impl->resourceRenderFinalize();
    impl->renderFinalize();
}

void Map::setMapConfigPath(const std::string &mapConfigPath,
                           const std::string &authPath)
{
    impl->setMapConfigPath(mapConfigPath, authPath);
}

const std::string Map::getMapConfigPath() const
{
    return impl->mapConfigPath;
}

void Map::purgeTraverseCache(bool hard)
{
    if (hard)
        impl->purgeHard();
    else
        impl->purgeSoft();
}

bool Map::isMapConfigReady() const
{
    return impl->mapConfig && *impl->mapConfig;
}

bool Map::isMapRenderComplete() const
{
    if (!isMapConfigReady())
        return false;
    return !impl->draws.draws.empty()
            && impl->statistics.currentNodeUpdates == 0;
}

double Map::getMapRenderProgress() const
{
    if (!isMapConfigReady())
        return 0;
    if (isMapRenderComplete())
        return 1;
    return 0.5; // todo some better heuristic
}

void Map::pan(const Point &value)
{
    if (!isMapConfigReady())
        return;
    impl->pan(vecFromPoint(value));
}

void Map::rotate(const Point &value)
{
    if (!isMapConfigReady())
        return;
    impl->rotate(vecFromPoint(value)); 
}

MapCallbacks &Map::callbacks()
{
    return impl->callbacks;
}

MapStatistics &Map::statistics()
{
    return impl->statistics;
}

MapOptions &Map::options()
{
    return impl->options;
}

MapDraws &Map::draws()
{
    return impl->draws;
}

MapCredits &Map::credits()
{
    return impl->credits;
}

void Map::setPositionSubjective(bool subjective, bool convert)
{
    if (!isMapConfigReady() || subjective == getPositionSubjective())
        return;
    if (convert)
        impl->convertPositionSubjObj();
    impl->mapConfig->position.type = subjective
            ? vtslibs::registry::Position::Type::subjective
            : vtslibs::registry::Position::Type::objective;
}

bool Map::getPositionSubjective() const
{
    if (!isMapConfigReady())
        return false;
    return impl->mapConfig->position.type
            == vtslibs::registry::Position::Type::subjective;
}

void Map::setPositionPoint(const Point &point)
{
    if (!isMapConfigReady())
        return;
    impl->mapConfig->position.position
            = vecToUblas<math::Point3>(vecFromPoint(point));
    impl->navigation.inertiaMotion = vec3(0,0,0);
}

const Point Map::getPositionPoint() const
{
    if (!isMapConfigReady())
        return vecToPoint(vec3(0,0,0));
    return vecToPoint(vecFromUblas<vec3>(impl->mapConfig->position.position));
}

void Map::setPositionRotation(const Point &point)
{
    if (!impl->mapConfig || !*impl->mapConfig)
        return;
    impl->mapConfig->position.orientation
            = vecToUblas<math::Point3>(vecFromPoint(point));
    impl->navigation.inertiaRotation = vec3(0,0,0);
}

const Point Map::getPositionRotation() const
{
    if (!isMapConfigReady())
        return vecToPoint(vec3(0,0,0));
    return vecToPoint(vecFromUblas<vec3>(
                          impl->mapConfig->position.orientation));
}

void Map::setPositionViewExtent(double viewExtent)
{
    if (!isMapConfigReady())
        return;
    impl->mapConfig->position.verticalExtent = viewExtent;
    impl->navigation.inertiaViewExtent = 0;
}

double Map::getPositionViewExtent() const
{
    if (!isMapConfigReady())
        return 0;
    return impl->mapConfig->position.verticalExtent;
}

void Map::setPositionFov(double fov)
{
    if (!isMapConfigReady())
        return;
    impl->mapConfig->position.verticalFov = fov;
}

double Map::getPositionFov() const
{
    if (!isMapConfigReady())
        return 0;
    return impl->mapConfig->position.verticalFov;
}

const std::string Map::getPositionJson() const
{
    if (!isMapConfigReady())
        return "";
    return Json::FastWriter().write(
                vtslibs::registry::asJson(impl->mapConfig->position));
}

void Map::setPositionJson(const std::string &position)
{
    if (!isMapConfigReady())
        return;
    Json::Value val;
    if (!Json::Reader().parse(position, val))
        LOGTHROW(err2, std::runtime_error) << "invalid position json";
    impl->mapConfig->position = vtslibs::registry::positionFromJson(val);
}

void Map::resetPositionAltitude()
{
    impl->resetPositionAltitude(0);
}

void Map::setAutorotate(double rotate)
{
    if (!isMapConfigReady())
        return;
    impl->mapConfig->autorotate = rotate;
}

double Map::getAutorotate() const
{
    if (!isMapConfigReady())
        return 0;
    return impl->mapConfig->autorotate;
}

const Point Map::convert(const Point &point, Srs from, Srs to) const
{
    if (!isMapConfigReady())
        return vecToPoint(vec3(0,0,0));
    return vecToPoint(impl->mapConfig->convertor->convert(vecFromPoint(point),
                srsConvert(impl->mapConfig.get(), from),
                srsConvert(impl->mapConfig.get(), to)));
}

const std::vector<std::string> Map::getResourceSurfaces() const
{
    if (!isMapConfigReady())
        return {};
    std::vector<std::string> names;
    names.reserve(impl->mapConfig->surfaces.size());
    for (auto &&it : impl->mapConfig->surfaces)
        names.push_back(it.id);
    return std::move(names);
}

const std::vector<std::string> Map::getResourceBoundLayers() const
{
    if (!isMapConfigReady())
        return {};
    std::vector<std::string> names;
    for (auto &&it : impl->mapConfig->boundLayers)
        names.push_back(it.id);
    return std::move(names);
}

const std::vector<std::string> Map::getResourceFreeLayers() const
{
    if (!isMapConfigReady())
        return {};
    std::vector<std::string> names;
    for (auto &&it : impl->mapConfig->freeLayers)
        names.push_back(it.first);
    return std::move(names);
}

const std::vector<std::string> Map::getViewNames() const
{
    if (!isMapConfigReady())
        return {};
    std::vector<std::string> names;
    names.reserve(impl->mapConfig->namedViews.size());
    for (auto &&it : impl->mapConfig->namedViews)
        names.push_back(it.first);
    return std::move(names);
}

const std::string Map::getViewCurrent() const
{
    if (!isMapConfigReady())
        return "";
    return impl->mapConfigView;
}

void Map::setViewCurrent(const std::string &name)
{
    if (!isMapConfigReady())
        return;
    auto it = impl->mapConfig->namedViews.find(name);
    if (it == impl->mapConfig->namedViews.end())
        LOGTHROW(err2, std::runtime_error) << "invalid mapconfig view name";
    impl->mapConfig->view = it->second;
    impl->purgeSoft();
    impl->mapConfigView = name;
}

void Map::getViewData(const std::string &name, MapView &view) const
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
        LOGTHROW(err2, std::runtime_error) << "invalid mapconfig view name";
    view = getMapView(it->second);
}

void Map::setViewData(const std::string &name, const MapView &view)
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

const std::string Map::getViewJson(const std::string &name) const
{
    if (!isMapConfigReady())
        return "";
    if (name == "")
        return Json::FastWriter().write(vtslibs::registry::asJson(
                impl->mapConfig->view, impl->mapConfig->boundLayers));
    auto it = impl->mapConfig->namedViews.find(name);
    if (it == impl->mapConfig->namedViews.end())
        LOGTHROW(err2, std::runtime_error) << "invalid mapconfig view name";
    return Json::FastWriter().write(vtslibs::registry::asJson(it->second,
                        impl->mapConfig->boundLayers));
}

void Map::setViewJson(const std::string &name,
                                const std::string &view)
{
    if (!isMapConfigReady())
        return;
    Json::Value val;
    if (!Json::Reader().parse(view, val))
        LOGTHROW(err2, std::runtime_error) << "invalid view json";
    if (name == "")
    {
        impl->mapConfig->view = vtslibs::registry::viewFromJson(val);
        impl->purgeSoft();
    }
    else
        impl->mapConfig->namedViews[name]
                = vtslibs::registry::viewFromJson(val);
}

void Map::printDebugInfo()
{
    impl->printDebugInfo();
}

MapImpl::MapImpl(Map *map, const MapCreateOptions &options) :
    map(map), initialized(false), resources(options)
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
    LOG(info3) << "Position: " << map->getPositionJson();
    LOG(info3) << "Named views: " << boost::join(map->getViewNames(), ", ");
    LOG(info3) << "Current view name: " << map->getViewCurrent();
    LOG(info3) << "Current view data: " << map->getViewJson("");
    mapConfig->printSurfaceStack();
}

} // namespace vts
