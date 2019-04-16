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

#include "../include/vts-browser/exceptions.hpp"
#include "../include/vts-browser/cameraDraws.hpp"
#include "../include/vts-browser/cameraOptions.hpp"
#include "../include/vts-browser/cameraStatistics.hpp"

#include "../navigation.hpp"
#include "../camera.hpp"
#include "../traverseNode.hpp"
#include "../mapConfig.hpp"
#include "../mapLayer.hpp"
#include "../authConfig.hpp"
#include "../credits.hpp"
#include "../coordsManip.hpp"
#include "../map.hpp"

namespace vts
{

void MapImpl::renderInitialize()
{
    LOG(info2) << "Render initialize";
}

void MapImpl::renderFinalize()
{
    LOG(info2) << "Render finalize";
}

void MapImpl::renderUpdate(double elapsedTime)
{
    lastElapsedFrameTime = elapsedTime;

    if (!prerequisitesCheck())
        return;

    assert(!resources.auth || *resources.auth);
    assert(mapconfig && *mapconfig);
    assert(convertor);
    assert(!layers.empty());
    assert(layers[0]->traverseRoot);

    updateSearch();

    cameras.erase(std::remove_if(cameras.begin(), cameras.end(),
        [&](std::weak_ptr<CameraImpl> &camera) {
        return !camera.lock();
    }), cameras.end());

    for (auto &it : layers)
        traverseClearing(it->traverseRoot.get());

    if (mapconfig->atmosphereDensityTexture)
        updateAtmosphereDensity();
}

void MapImpl::initializeNavigation()
{
    for (auto &camera : cameras)
    {
        auto cam = camera.lock();
        if (cam)
        {
            auto nav = cam->navigation.lock();
            if (nav)
                nav->initialize();
        }
    }
}

void MapImpl::purgeMapconfig()
{
    LOG(info2) << "Purge mapconfig";

    if (resources.auth)
        resources.auth->forceRedownload();
    resources.auth.reset();
    if (mapconfig)
        mapconfig->forceRedownload();
    mapconfig.reset();
    mapconfigAvailable = false;

    credits->purge();
    resources.searchTasks.clear();
    convertor.reset();
    body = MapCelestialBody();
    purgeViewCache();

    for (auto &camera : cameras)
    {
        auto cam = camera.lock();
        if (cam)
        {
            auto nav = cam->navigation.lock();
            if (nav)
            {
                nav->autoRotation = 0;
                nav->resetNavigationMode();
                nav->lastPositionAltitude.reset();
                nav->positionAltitudeReset.reset();
            }
        }
    }
}

void MapImpl::purgeViewCache()
{
    LOG(info2) << "Purge view cache";

    if (mapconfig)
        mapconfig->consolidateView();
    mapconfigReady = false;
    mapconfigView = "";
    layers.clear();

    for (auto &camera : cameras)
    {
        auto cam = camera.lock();
        if (cam)
        {
            cam->statistics = CameraStatistics();
            cam->draws = CameraDraws();
            cam->credits = CameraCredits();
        }
    }
}

void MapImpl::setMapconfigPath(const std::string &mapconfigPath,
    const std::string &authPath)
{
    LOG(info3) << "Changing mapconfig path to <" << mapconfigPath << ">, "
        << (!authPath.empty() ? "using" : "without")
        << " authentication";
    this->mapconfigPath = mapconfigPath;
    resources.authPath = authPath;
    purgeMapconfig();
}

bool MapImpl::prerequisitesCheck()
{
    if (resources.auth)
        resources.auth->checkTime();

    if (mapconfigReady)
        return true;

    if (mapconfigPath.empty())
        return false;

    if (!resources.authPath.empty())
    {
        resources.auth = getAuthConfig(resources.authPath);
        if (!testAndThrow(resources.auth->state, "Authentication failure."))
            return false;
    }

    mapconfig = getMapconfig(mapconfigPath);
    if (!testAndThrow(mapconfig->state, "Mapconfig failure."))
        return false;

    if (!mapconfigAvailable)
    {
        convertor = CoordManip::create(
            *mapconfig,
            mapconfig->browserOptions.searchSrs,
            createOptions.customSrs1,
            createOptions.customSrs2);

        credits->merge(mapconfig.get());
        initializeNavigation();
        mapconfig->initializeCelestialBody();

        LOG(info3) << "Mapconfig is available.";
        mapconfigAvailable = true;
        if (callbacks.mapconfigAvailable)
        {
            // this may change mapconfigAvailable
            callbacks.mapconfigAvailable();
            return false;
        }
    }

    if (layers.empty())
    {
        // main surface stack
        layers.push_back(std::make_shared<MapLayer>(this));

        // free layers
        for (const auto &it : mapconfig->view.freeLayers)
            layers.push_back(std::make_shared<MapLayer>(this, it.first,
                it.second));
    }

    // check all layers
    {
        bool ok = true;
        for (auto &it : layers)
        {
            if (!it->prerequisitesCheck())
                ok = false;
        }
        if (!ok)
            return false;
    }

    LOG(info3) << "Mapconfig is ready.";
    mapconfigReady = true;
    if (callbacks.mapconfigReady)
        callbacks.mapconfigReady(); // this may change mapconfigReady
    return mapconfigReady;
}

void MapImpl::traverseClearing(TraverseNode *trav)
{
    if (std::max(trav->lastAccessTime, trav->lastRenderTime) + 5
                < renderTickIndex)
    {
        if (trav->meta)
            trav->clearAll();
        assert(trav->childs.empty());
        assert(trav->rendersEmpty());
        assert(!trav->surface);
        assert(!trav->determined);
        return;
    }

    if (trav->lastRenderTime + 5 < renderTickIndex)
    {
        if (trav->determined)
            trav->clearRenders();
        assert(trav->rendersEmpty());
        assert(!trav->determined);
    }

    for (auto &it : trav->childs)
        traverseClearing(it.get());
}

TileId MapImpl::roundId(TileId nodeId)
{
    uint32 metaTileBinaryOrder = mapconfig->referenceFrame.metaBinaryOrder;
    return TileId (nodeId.lod,
       (nodeId.x >> metaTileBinaryOrder) << metaTileBinaryOrder,
       (nodeId.y >> metaTileBinaryOrder) << metaTileBinaryOrder);
}

bool testAndThrow(Resource::State state, const std::string &message)
{
    switch (state)
    {
    case Resource::State::initializing:
    case Resource::State::checkCache:
    case Resource::State::startDownload:
    case Resource::State::downloaded:
    case Resource::State::downloading:
    case Resource::State::errorRetry:
        return false;
    case Resource::State::ready:
        return true;
    default:
        LOGTHROW(err4, MapconfigException) << message;
        throw;
    }
}

std::string convertPath(const std::string &path,
                        const std::string &parent)
{
    if (path.empty())
        return "";
    assert(!parent.empty());
    return utility::Uri(parent).resolve(path).str();
}

TraverseNode *findTravById(TraverseNode *trav, const TileId &what)
{
    if (!trav)
        return nullptr;
    TileId id = trav->id();
    if (id == what)
        return trav;
    if (what.lod <= id.lod)
        return findTravById(trav->parent, what);
    TileId t = what;
    while (t.lod > id.lod + 1)
        t = vtslibs::vts::parent(t);
    for (auto &it : trav->childs)
        if (it->id() == t)
            return findTravById(it.get(), what);
    return nullptr;
}

} // namespace vts

