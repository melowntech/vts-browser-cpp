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

#include "../include/vts-browser/map.hpp"
#include "../include/vts-browser/view.hpp"
#include "../utilities/json.hpp"
#include "../resources/cache.hpp"
#include "../map.hpp"
#include "../mapConfig.hpp"
#include "../gpuResource.hpp"
#include "../renderInfos.hpp"
#include "../coordsManip.hpp"
#include "../credits.hpp"
#include "../geodata.hpp"

namespace vts
{

namespace
{

void getMapViewBoundLayers(
        const vtslibs::registry::View::BoundLayerParams::list &in,
        MapView::BoundLayerInfo::List &out)
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
        const MapView::BoundLayerInfo::List &in,
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
        f.styleUrl = it.second.style ? *it.second.style : "";
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
        if (!it.second.styleUrl.empty())
            f.style = it.second.styleUrl;
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

Map::Map() :
    Map(MapCreateOptions(), Fetcher::create(FetcherOptions()))
{}

Map::Map(const MapCreateOptions &options) : 
    Map(options, Fetcher::create(FetcherOptions()))
{}

Map::Map(const MapCreateOptions &options,
    const std::shared_ptr<Fetcher> &fetcher)
{
    LOG(info3) << "Creating map";
    impl = std::make_shared<MapImpl>(this, options, fetcher);
}

Map::~Map()
{
    LOG(info3) << "Destroying map";
}

void Map::setMapconfigPath(const std::string &mapconfigPath,
    const std::string &authPath)
{
    impl->setMapconfigPath(mapconfigPath, authPath);
}

std::string Map::getMapconfigPath() const
{
    return impl->mapconfigPath;
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

bool Map::getMapconfigAvailable() const
{
    if (impl->mapconfigAvailable)
    {
        assert(impl->mapconfig);
        assert(*impl->mapconfig);
        assert(impl->convertor);
    }
    return impl->mapconfigAvailable;
}

bool Map::getMapconfigReady() const
{
    if (impl->mapconfigReady)
    {
        assert(getMapconfigAvailable());
    }
    return impl->mapconfigReady;
}

bool Map::getMapProjected() const
{
    if (getMapconfigAvailable())
        return impl->mapconfig->navigationSrsType()
            == vtslibs::registry::Srs::Type::projected;
    return false;
}

bool Map::getMapRenderComplete() const
{
    if (!getMapconfigReady())
        return false;
    return impl->getMapRenderComplete();
}

double Map::getMapRenderProgress() const
{
    if (!getMapconfigReady())
        return 0;
    if (getMapRenderComplete())
        return 1;
    return std::min(impl->getMapRenderProgress(), 0.999);
}

void Map::dataInitialize()
{
    impl->resourceDataInitialize();
}

void Map::dataUpdate()
{
    impl->resourceDataUpdate();
}

void Map::dataFinalize()
{
    impl->resourceDataFinalize();
}

void Map::dataAllRun()
{
    impl->resourceDataInitialize();
    impl->resourceDataRun();
    impl->resourceDataFinalize();
}

void Map::renderInitialize()
{
    impl->resourceRenderInitialize();
    impl->renderInitialize();
}

void Map::renderUpdate(double elapsedTime)
{
    impl->statistics.renderTicks = ++impl->renderTickIndex;
    impl->resourceRenderUpdate();
    impl->renderUpdate(elapsedTime);
}

void Map::renderFinalize()
{
    impl->resourceRenderFinalize();
    impl->renderFinalize();
}

double Map::lastRenderUpdateElapsedTime() const
{
    return impl->lastElapsedFrameTime;
}

MapCallbacks &Map::callbacks()
{
    return impl->callbacks;
}

MapStatistics &Map::statistics()
{
    return impl->statistics;
}

MapRuntimeOptions &Map::options()
{
    return impl->options;
}

const MapCelestialBody &Map::celestialBody()
{
    return impl->body;
}

std::shared_ptr<void> Map::atmosphereDensityTexture()
{
    if (getMapconfigAvailable() && impl->mapconfig->atmosphereDensityTexture
        && *impl->mapconfig->atmosphereDensityTexture)
        return impl->mapconfig->atmosphereDensityTexture->info.userData;
    return nullptr;
}

void Map::convert(const double pointFrom[3], double pointTo[3],
                  Srs srsFrom, Srs srsTo) const
{
    if (!getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    vec3 a = rawToVec3(pointFrom);
    a = impl->convertor->convert(a, srsFrom, srsTo);
    vecToRaw(a, pointTo);
}

void Map::convert(const std::array<double, 3> &pointFrom, double pointTo[3],
            Srs srsFrom, Srs srsTo) const
{
    convert(pointFrom.data(), pointTo, srsFrom, srsTo);
}

std::vector<std::string> Map::getResourceSurfaces() const
{
    if (!getMapconfigAvailable())
        return {};
    std::vector<std::string> names;
    names.reserve(impl->mapconfig->surfaces.size());
    for (auto &it : impl->mapconfig->surfaces)
        names.push_back(it.id);
    return names;
}

std::vector<std::string> Map::getResourceBoundLayers() const
{
    if (!getMapconfigAvailable())
        return {};
    std::vector<std::string> names;
    for (auto &it : impl->mapconfig->boundLayers)
        names.push_back(it.id);
    return names;
}

std::vector<std::string> Map::getResourceFreeLayers() const
{
    if (!getMapconfigAvailable())
        return {};
    std::vector<std::string> names;
    for (auto &it : impl->mapconfig->freeLayers)
        names.push_back(it.first);
    return names;
}

FreeLayerType Map::getResourceFreeLayerType(const std::string &name) const
{
    if (!getMapconfigAvailable())
        return FreeLayerType::Unknown;
    if (!impl->mapconfig->freeLayers.has(name))
        return FreeLayerType::Unknown;
    auto *f = impl->mapconfig->getFreeInfo(name);
    if (!f)
        return FreeLayerType::Unknown;
    switch (f->type)
    {
    case vtslibs::registry::FreeLayer::Type::meshTiles:
        return FreeLayerType::TiledMeshes;
    case vtslibs::registry::FreeLayer::Type::geodataTiles:
        return FreeLayerType::TiledGeodata;
    case vtslibs::registry::FreeLayer::Type::geodata:
        return FreeLayerType::MonolithicGeodata;
    default:
        return FreeLayerType::Unknown;
    }
}

void Map::fabricateResourceFreeLayerGeodata(const std::string &name)
{
    if (!getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    if (impl->mapconfig->freeLayers.has(name))
    {
        LOGTHROW(err4, std::logic_error)
                << "Duplicate free layer name.";
    }
    vtslibs::registry::FreeLayer fl;
    fl.id = name;
    auto &g = fl.createDefinition<vtslibs::registry::FreeLayer::Geodata>();
    g.displaySize = 0;
    impl->mapconfig->freeLayers.set(name, fl);
    impl->mapconfig->getFreeInfo(name);
}

std::string Map::getResourceFreeLayerGeodata(const std::string &name) const
{
    if (getResourceFreeLayerType(name) != FreeLayerType::MonolithicGeodata)
        return "";
    auto r = impl->getActualGeoFeatures(name);
    if (r.second)
        return *r.second;
    return "";
}

void Map::setResourceFreeLayerGeodata(const std::string &name,
                                      const std::string &value)
{
    if (getResourceFreeLayerType(name) != FreeLayerType::MonolithicGeodata)
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    auto &v = impl->mapconfig->getFreeInfo(name)->overrideGeodata;
    if (!v || *v != value)
    {
        if (value.empty())
            v.reset();
        else
            v = std::make_shared<const std::string>(value);
    }
    purgeViewCache();
}

std::string Map::getResourceFreeLayerStyle(const std::string &name) const
{
    switch (getResourceFreeLayerType(name))
    {
    case FreeLayerType::MonolithicGeodata:
    case FreeLayerType::TiledGeodata:
        break;
    default:
        return "";
    }
    auto r = impl->getActualGeoStyle(name);
    if (r.second)
        return r.second->data;
    return "";
}

void Map::setResourceFreeLayerStyle(const std::string &name,
                                    const std::string &value)
{
    switch (getResourceFreeLayerType(name))
    {
    case FreeLayerType::MonolithicGeodata:
    case FreeLayerType::TiledGeodata:
        break;
    default:
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
        throw;
    }
    auto &v = impl->mapconfig->getFreeInfo(name)->stylesheet;
    v = impl->newGeoStyle(value);
    purgeViewCache();
}

std::vector<std::string> Map::getViewNames() const
{
    if (!getMapconfigAvailable())
        return {};
    std::vector<std::string> names;
    names.reserve(impl->mapconfig->namedViews.size());
    for (auto &it : impl->mapconfig->namedViews)
        names.push_back(it.first);
    return names;
}

std::string Map::getViewCurrent() const
{
    if (!getMapconfigAvailable())
        return "";
    return impl->mapconfigView;
}

void Map::setViewCurrent(const std::string &name)
{
    if (!getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    auto it = impl->mapconfig->namedViews.find(name);
    if (it == impl->mapconfig->namedViews.end())
    {
        LOGTHROW(err2, std::runtime_error)
                << "Specified mapconfig view could not be found.";
    }
    impl->mapconfig->view = it->second;
    impl->purgeViewCache();
    impl->mapconfigView = name;
}

MapView Map::getViewData(const std::string &name) const
{
    if (!getMapconfigAvailable())
        return {};
    auto it = impl->mapconfig->namedViews.find(name);
    if (it == impl->mapconfig->namedViews.end())
    {
        LOGTHROW(err2, std::runtime_error)
                << "Specified mapconfig view could not be found.";
    }
    return getMapView(it->second);
}

void Map::setViewData(const std::string &name, const MapView &view)
{
    if (!getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    impl->mapconfig->namedViews[name] = setMapView(view);
    if (name == getViewCurrent())
    {
        impl->mapconfig->view = impl->mapconfig->namedViews[name];
        impl->purgeViewCache();
    }
}

void Map::removeView(const std::string &name)
{
    if (!getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    if (name == getViewCurrent())
    {
        LOGTHROW(err2, std::runtime_error)
                << "Named mapconfig view cannot be erased "
                   "because it is beeing used.";
    }
    impl->mapconfig->namedViews.erase(name);
}

std::string Map::getViewJson(const std::string &name) const
{
    if (!getMapconfigAvailable())
        return "";
    if (name == "")
        return jsonToString(vtslibs::registry::asJson(
                impl->mapconfig->view, impl->mapconfig->boundLayers));
    auto it = impl->mapconfig->namedViews.find(name);
    if (it == impl->mapconfig->namedViews.end())
    {
        LOGTHROW(err2, std::runtime_error)
                << "Specified mapconfig view could not be found.";
    }
    return jsonToString(vtslibs::registry::asJson(it->second,
                        impl->mapconfig->boundLayers));
}

void Map::setViewJson(const std::string &name,
                                const std::string &view)
{
    if (!getMapconfigAvailable())
    {
        LOGTHROW(err4, std::logic_error)
                << "Map is not yet available.";
    }
    Json::Value val = stringToJson(view);
    setViewData(name, getMapView(vtslibs::registry::viewFromJson(val)));
}

bool Map::searchable() const
{
    if (!getMapconfigAvailable())
        return false;
    return !impl->mapconfig->browserOptions.searchUrl.empty();
}

std::shared_ptr<SearchTask> Map::search(const std::string &query)
{
    return search(query, impl->mapconfig->position.position.data().begin());
}

std::shared_ptr<SearchTask> Map::search(const std::string &query,
                                        const double point[3])
{
    if (!getMapconfigAvailable())
        return {};
    return impl->search(query, point);
}

std::shared_ptr<SearchTask> Map::search(const std::string &query,
                                const std::array<double, 3> &point)
{
    return search(query, point.data());
}

MapImpl::MapImpl(Map *map, const MapCreateOptions &options,
                    const std::shared_ptr<Fetcher> &fetcher) :
    map(map), createOptions(options),
    lastElapsedFrameTime(0),
    renderTickIndex(0),
    mapconfigAvailable(false), mapconfigReady(false)
{
    assert(fetcher);
    resources.fetcher = fetcher;
    resources.cache = Cache::create(options);
    resources.thrCacheReader = std::thread(&MapImpl::cacheReadEntry, this);
    resources.thrCacheWriter = std::thread(&MapImpl::cacheWriteEntry, this);
    resources.thrAtmosphereGenerator
            = std::thread(&MapImpl::resourcesAtmosphereGeneratorEntry, this);
    resources.thrGeodataProcessor
            = std::thread(&MapImpl::resourcesGeodataProcessorEntry, this);
    resources.fetching.thr
            = std::thread(&MapImpl::resourcesDownloadsEntry, this);
    credits = std::make_shared<Credits>();
}

MapImpl::~MapImpl()
{
    resources.queCacheRead.terminate();
    resources.queCacheWrite.terminate();
    resources.queUpload.terminate();
    resources.queAtmosphere.terminate();
    resources.queGeodata.terminate();
    resources.thrCacheReader.join();
    resources.thrCacheWriter.join();
    resources.thrAtmosphereGenerator.join();
    resources.thrGeodataProcessor.join();
    resources.fetching.stop = true;
    resources.fetching.con.notify_all();
    resources.fetching.thr.join();
}

} // namespace vts
