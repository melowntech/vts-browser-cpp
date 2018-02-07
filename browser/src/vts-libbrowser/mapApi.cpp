/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <vts-libs/registry/json.hpp>
#include <vts-libs/registry/io.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "include/vts-browser/map.hpp"
#include "include/vts-browser/view.hpp"
#include "map.hpp"

namespace vts
{

namespace
{

void getMapViewBoundLayers(
        const vtslibs::registry::View::BoundLayerParams::list &in,
        MapView::BoundLayerInfo::Map &out)
{
    out.clear();
    for (auto &it : in)
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
    for (auto &it : in)
    {
        vtslibs::registry::View::BoundLayerParams b;
        b.id = it.id;
        if (it.alpha < 1 - 1e-7)
            b.alpha = it.alpha;
        out.push_back(b);
    }
}

MapView getMapView(const vtslibs::registry::View &view)
{
    MapView value;
    value.description = view.description ? *view.description : "";
    for (auto &it : view.surfaces)
    {
        MapView::SurfaceInfo &s = value.surfaces[it.first];
        getMapViewBoundLayers(it.second, s.boundLayers);
    }
    for (auto &it : view.freeLayers)
    {
        MapView::FreeLayerInfo &f = value.freeLayers[it.first];
        f.style = it.second.style ? *it.second.style : "";
        getMapViewBoundLayers(it.second.boundLayers, f.boundLayers);
    }
    return value;
}

vtslibs::registry::View setMapView(const MapView &value)
{
    vtslibs::registry::View view;
    if (!value.description.empty())
        view.description = value.description;
    for (auto &it : value.surfaces)
    {
        vtslibs::registry::View::BoundLayerParams::list &b
                = view.surfaces[it.first];
        setMapViewBoundLayers(it.second.boundLayers, b);
    }
    for (auto &it : value.freeLayers)
    {
        vtslibs::registry::View::FreeLayerParams &f
                = view.freeLayers[it.first];
        if (!it.second.style.empty())
            f.style = it.second.style;
        setMapViewBoundLayers(it.second.boundLayers, f.boundLayers);
    }
    return view;
}

} // namespace

MapView::BoundLayerInfo::BoundLayerInfo() : alpha(1)
{}

MapView::BoundLayerInfo::BoundLayerInfo(const std::string &id) :
    id(id), alpha(1)
{}

Map::Map(const MapCreateOptions &options)
{
    LOG(info3) << "Creating map";
    impl = std::shared_ptr<MapImpl>(new MapImpl(this, options));
}

Map::~Map()
{
    LOG(info3) << "Destroying map";
}

void Map::dataInitialize(const std::shared_ptr<Fetcher> &fetcher)
{
    impl->resourceDataInitialize(fetcher);
}

void Map::dataTick()
{
    impl->statistics.dataTicks = ++impl->resources.tickIndex;
    impl->resourceDataTick();
}

void Map::dataFinalize()
{
    impl->resourceDataFinalize();
}

void Map::renderInitialize()
{
    impl->resourceRenderInitialize();
    impl->renderInitialize();
}

void Map::renderTickPrepare()
{
    impl->statistics.resetFrame();
    impl->statistics.renderTicks = ++impl->renderer.tickIndex;
    impl->resourceRenderTick();
    impl->renderTickPrepare();
}

void Map::renderTickRender()
{
    impl->renderTickRender();
}

void Map::renderFinalize()
{
    impl->resourceRenderFinalize();
    impl->renderFinalize();
}

void Map::setWindowSize(uint32 width, uint32 height)
{
    impl->renderer.windowWidth = width;
    impl->renderer.windowHeight = height;
}

void Map::setMapConfigPath(const std::string &mapConfigPath,
                           const std::string &authPath,
                           const std::string &sriPath)
{
    impl->setMapConfigPath(mapConfigPath, authPath, sriPath);
    impl->initiateSri(nullptr);
}

std::string &Map::getMapConfigPath() const
{
    return impl->mapConfigPath;
}

void Map::purgeDiskCache()
{
    if (impl->resources.cache)
        impl->resources.cache->purge();
}

void Map::purgeViewCache()
{
    impl->purgeViewCache();
}

bool Map::getMapProjected() const
{
    if (!getMapConfigReady())
        return false;
    return impl->mapConfig->navigationSrsType()
            == vtslibs::registry::Srs::Type::projected;
}

bool Map::getMapConfigReady() const
{
    if (impl->initialized)
    {
        assert(impl->mapConfig);
        assert(*impl->mapConfig);
        assert(impl->convertor);
    }
    return impl->initialized;
}

bool Map::getMapRenderComplete() const
{
    if (!getMapConfigReady())
        return false;
    return impl->statistics.currentNodeMetaUpdates == 0
        && impl->statistics.currentNodeDrawsUpdates == 0
        && impl->statistics.currentResourcePreparing == 0;
}

double Map::getMapRenderProgress() const
{
    if (!getMapConfigReady())
        return 0;
    if (getMapRenderComplete())
        return 1;
    return impl->getMapRenderProgress();
}

void Map::pan(const double value[3])
{
    if (!getMapConfigReady())
        return;
    if (impl->mapConfig->position.type
            == vtslibs::registry::Position::Type::objective)
        impl->pan(rawToVec3(value));
    else
        impl->rotate(-rawToVec3(value));
}

void Map::pan(const double (&value)[3])
{
    pan(&value[0]);
}

void Map::rotate(const double value[3])
{
    if (!getMapConfigReady())
        return;
    impl->rotate(rawToVec3(value));
}

void Map::rotate(const double (&value)[3])
{
    rotate(&value[0]);
}

void Map::zoom(double value)
{
    if (!getMapConfigReady())
        return;
    if (impl->mapConfig->position.type
            == vtslibs::registry::Position::Type::objective)
        impl->zoom(value);
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

const MapCredits &Map::credits()
{
    return impl->credits;
}

const MapCelestialBody &Map::celestialBody()
{
    return impl->body;
}

void Map::setPositionSubjective(bool subjective, bool convert)
{
    if (!getMapConfigReady() || subjective == getPositionSubjective())
        return;
    if (convert)
        impl->convertPositionSubjObj();
    impl->mapConfig->position.type = subjective
            ? vtslibs::registry::Position::Type::subjective
            : vtslibs::registry::Position::Type::objective;
}

bool Map::getPositionSubjective() const
{
    if (!getMapConfigReady())
        return false;
    return impl->mapConfig->position.type
            == vtslibs::registry::Position::Type::subjective;
}

void Map::setPositionPoint(const double point[3])
{
    if (!getMapConfigReady())
        return;
    assert(point[0] == point[0]);
    assert(point[1] == point[1]);
    assert(point[2] == point[2]);
    if (impl->mapConfig->navigationSrsType()
                   == vtslibs::registry::Srs::Type::geographic)
    {
        assert(point[0] >= -180 && point[0] <= 180);
        assert(point[1] >= -90 && point[1] <= 90);
    }
    vec3 v = rawToVec3(point);
    impl->setPoint(v);
    if (impl->options.enableArbitrarySriRequests
            && impl->convertor->geoDistance(
                vecFromUblas<vec3>(impl->mapConfig->position.position), v)
            > impl->mapConfig->position.verticalExtent * 3)
    {
        vtslibs::registry::Position p = impl->mapConfig->position;
        p.position = vecToUblas<math::Point3>(v);
        impl->initiateSri(&p);
    }
}

void Map::setPositionPoint(const double (&point)[3])
{
    setPositionPoint(&point[0]);
}

void Map::getPositionPoint(double point[3]) const
{
    if (!getMapConfigReady())
    {
        for (int i = 0; i < 3; i++)
            point[i] = 0;
        return;
    }
    auto p = impl->mapConfig->position.position;
    vecToRaw(vecFromUblas<vec3>(p), point);
}

void Map::setPositionRotation(const double point[3])
{
    if (!impl->mapConfig || !*impl->mapConfig)
        return;
    assert(point[0] == point[0]);
    assert(point[1] == point[1]);
    assert(point[2] == point[2]);
    impl->setRotation(rawToVec3(point));
}

void Map::setPositionRotation(const double (&point)[3])
{
    setPositionRotation(&point[0]);
}

void Map::getPositionRotation(double point[3]) const
{
    if (!getMapConfigReady())
    {
        for (int i = 0; i < 3; i++)
            point[i] = 0;
        return;
    }
    auto p = impl->mapConfig->position.orientation;
    vecToRaw(vecFromUblas<vec3>(p), point);
}

void Map::getPositionRotationLimited(double point[3]) const
{
    vec3 p = vecFromUblas<vec3>(impl->mapConfig->position.orientation);
    impl->applyCameraRotationNormalization(p);
    vecToRaw(p, point);
}

void Map::setPositionViewExtent(double viewExtent)
{
    if (!getMapConfigReady())
        return;
    assert(viewExtent == viewExtent);
    impl->setViewExtent(viewExtent);
}

double Map::getPositionViewExtent() const
{
    if (!getMapConfigReady())
        return 0;
    return impl->mapConfig->position.verticalExtent;
}

void Map::setPositionFov(double fov)
{
    if (!getMapConfigReady())
        return;
    assert(fov > 0 && fov < 180);
    impl->mapConfig->position.verticalFov = fov;
}

double Map::getPositionFov() const
{
    if (!getMapConfigReady())
        return 0;
    return impl->mapConfig->position.verticalFov;
}

std::string Map::getPositionJson() const
{
    if (!getMapConfigReady())
        return "";
    return Json::FastWriter().write(
                vtslibs::registry::asJson(impl->mapConfig->position));
}

std::string Map::getPositionUrl() const
{
    if (!getMapConfigReady())
        return "";
    return boost::lexical_cast<std::string>(impl->mapConfig->position);
}

namespace
{

void setPosition(Map *map, const vtslibs::registry::Position &pos)
{
    if (pos.heightMode != vtslibs::registry::Position::HeightMode::fixed)
        LOGTHROW(err2, std::runtime_error) << "position must have fixed height";
    // todo height mode
    map->setPositionSubjective(
            pos.type == vtslibs::registry::Position::Type::subjective, false);
    map->setPositionFov(pos.verticalFov);
    map->setPositionViewExtent(pos.verticalExtent);
    vec3 v = vecFromUblas<vec3>(pos.orientation);
    map->setPositionRotation(v.data());
    v = vecFromUblas<vec3>(pos.position);
    map->setPositionPoint(v.data());
}

} // namespace

void Map::setPositionJson(const std::string &position)
{
    if (!getMapConfigReady())
        return;
    Json::Value val;
    if (!Json::Reader().parse(position, val))
        LOGTHROW(err2, std::runtime_error) << "invalid position from json";
    vtslibs::registry::Position pos = vtslibs::registry::positionFromJson(val);
    setPosition(this, pos);
}

void Map::setPositionUrl(const std::string &position)
{
    if (!getMapConfigReady())
        return;
    vtslibs::registry::Position pos;
    try
    {
        pos = boost::lexical_cast<vtslibs::registry::Position>(position);
    }
    catch(...)
    {
        LOGTHROW(err2, std::runtime_error) << "invalid position from url";
    }
    setPosition(this, pos);
}

void Map::resetPositionAltitude()
{
    if (!getMapConfigReady())
        return;
    impl->navigation.positionAltitudeReset = 0;
    impl->updatePositionAltitude();
}

void Map::resetNavigationMode()
{
    if (!getMapConfigReady())
        return;
    impl->resetNavigationMode();
}

void Map::setAutoRotation(double value)
{
    if (!getMapConfigReady())
        return;
    impl->navigation.autoRotation = value;
}

double Map::getAutoRotation() const
{
    if (!getMapConfigReady())
        return 0;
    return impl->navigation.autoRotation;
}

void Map::convert(const double pointFrom[3], double pointTo[3],
                  Srs srsFrom, Srs srsTo) const
{
    if (!getMapConfigReady())
        return;
    vec3 a = rawToVec3(pointFrom);
    a = impl->convertor->convert(a, srsFrom, srsTo);
    vecToRaw(a, pointTo);
}

std::vector<std::string> Map::getResourceSurfaces() const
{
    if (!getMapConfigReady())
        return {};
    std::vector<std::string> names;
    names.reserve(impl->mapConfig->surfaces.size());
    for (auto &it : impl->mapConfig->surfaces)
        names.push_back(it.id);
    return names;
}

std::vector<std::string> Map::getResourceBoundLayers() const
{
    if (!getMapConfigReady())
        return {};
    std::vector<std::string> names;
    for (auto &it : impl->mapConfig->boundLayers)
        names.push_back(it.id);
    return names;
}

std::vector<std::string> Map::getResourceFreeLayers() const
{
    if (!getMapConfigReady())
        return {};
    std::vector<std::string> names;
    for (auto &it : impl->mapConfig->freeLayers)
        names.push_back(it.first);
    return names;
}

std::vector<std::string> Map::getViewNames() const
{
    if (!getMapConfigReady())
        return {};
    std::vector<std::string> names;
    names.reserve(impl->mapConfig->namedViews.size());
    for (auto &it : impl->mapConfig->namedViews)
        names.push_back(it.first);
    return names;
}

std::string Map::getViewCurrent() const
{
    if (!getMapConfigReady())
        return "";
    return impl->mapConfigView;
}

void Map::setViewCurrent(const std::string &name)
{
    if (!getMapConfigReady())
        return;
    auto it = impl->mapConfig->namedViews.find(name);
    if (it == impl->mapConfig->namedViews.end())
        LOGTHROW(err2, std::runtime_error)
                << "specified mapconfig view could not be found";
    impl->mapConfig->view = it->second;
    impl->purgeViewCache();
    impl->mapConfigView = name;
    if (impl->options.enableArbitrarySriRequests)
        impl->initiateSri(nullptr);
}

void Map::getViewData(const std::string &name, MapView &view) const
{
    if (!getMapConfigReady())
        return;
    auto it = impl->mapConfig->namedViews.find(name);
    if (it == impl->mapConfig->namedViews.end())
        LOGTHROW(err2, std::runtime_error)
                << "specified mapconfig view could not be found";
    view = getMapView(it->second);
}

void Map::setViewData(const std::string &name, const MapView &view)
{
    if (!getMapConfigReady())
        return;
    impl->mapConfig->namedViews[name] = setMapView(view);
    if (name == getViewCurrent())
    {
        impl->mapConfig->view = impl->mapConfig->namedViews[name];
        impl->purgeViewCache();
    }
}

void Map::removeView(const std::string &name)
{
    if (!getMapConfigReady())
        return;
    if (name == getViewCurrent())
        LOGTHROW(err2, std::runtime_error)
                << "named mapconfig view cannot be erased, "
                   "because it is beeing used";
    impl->mapConfig->namedViews.erase(name);
}

std::string Map::getViewJson(const std::string &name) const
{
    if (!getMapConfigReady())
        return "";
    if (name == "")
        return Json::FastWriter().write(vtslibs::registry::asJson(
                impl->mapConfig->view, impl->mapConfig->boundLayers));
    auto it = impl->mapConfig->namedViews.find(name);
    if (it == impl->mapConfig->namedViews.end())
        LOGTHROW(err2, std::runtime_error)
                << "specified mapconfig view could not be found";
    return Json::FastWriter().write(vtslibs::registry::asJson(it->second,
                        impl->mapConfig->boundLayers));
}

void Map::setViewJson(const std::string &name,
                                const std::string &view)
{
    if (!getMapConfigReady())
        return;
    Json::Value val;
    Json::Reader r;
    if (!r.parse(view, val))
        LOGTHROW(err2, std::runtime_error) << "invalid view json: "
                                           << r.getFormattedErrorMessages();
    setViewData(name, getMapView(vtslibs::registry::viewFromJson(val)));
}

bool Map::searchable() const
{
    if (!getMapConfigReady())
        return false;
    return !impl->mapConfig->browserOptions.searchUrl.empty();
}

std::shared_ptr<SearchTask> Map::search(const std::string &query)
{
    return search(query, impl->mapConfig->position.position.data().begin());
}

std::shared_ptr<SearchTask> Map::search(const std::string &query,
                                        const double point[3])
{
    if (!getMapConfigReady())
        return {};
    return impl->search(query, point);
}

std::shared_ptr<SearchTask> Map::search(const std::string &query,
                                        const double (&point)[3])
{
    return search(query, &point[0]);
}

void Map::printDebugInfo()
{
    impl->printDebugInfo();
}

MapImpl::MapImpl(Map *map, const MapCreateOptions &options) :
    map(map), createOptions(options), initialized(false)
{
    resources.cache = Cache::create(options);
}

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
