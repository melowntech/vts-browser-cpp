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

#include "map.hpp"

namespace vts
{

MapImpl::Renderer::Renderer() :
    windowWidth(0), windowHeight(0), tickIndex(0)
{}

void MapImpl::renderInitialize()
{
    LOG(info3) << "Render initialize";
}

void MapImpl::renderFinalize()
{
    LOG(info3) << "Render finalize";
}

void MapImpl::setMapConfigPath(const std::string &mapConfigPath,
                               const std::string &authPath,
                               const std::string &sriPath)
{
    LOG(info3) << "Changing map config path to <" << mapConfigPath << ">, "
               << (!authPath.empty() ? "using" : "without")
               << " authentication and "
               << (!sriPath.empty() ?
                       std::string() + "using SRI <" + sriPath + ">"
                     : "without SRI");
    this->mapConfigPath = mapConfigPath;
    resources.authPath = authPath;
    resources.sriPath = sriPath;
    purgeMapConfig();
}

void MapImpl::purgeMapConfig()
{
    LOG(info2) << "Purge map config";

    if (resources.auth)
    {
        resources.auth->state = Resource::State::errorRetry;
        resources.auth->retryTime = 0;
    }
    if (mapConfig)
    {
        mapConfig->state = Resource::State::errorRetry;
        mapConfig->retryTime = 0;
    }

    resources.auth.reset();
    mapConfig.reset();
    renderer.credits.purge();
    resources.searchTasks.clear();
    resetNavigationMode();
    navigation.autoRotation = 0;
    navigation.lastPositionAltitude.reset();
    navigation.positionAltitudeReset.reset();
    body = MapCelestialBody();
    purgeViewCache();
}

void MapImpl::purgeViewCache()
{
    LOG(info2) << "Purge view cache";

    if (mapConfig)
        mapConfig->consolidateView();

    layers.clear();
    statistics.resetFrame();
    draws = MapDraws();
    credits = MapCredits();
    mapConfigView = "";
    initialized = false;
}

TileId MapImpl::roundId(TileId nodeId)
{
    uint32 metaTileBinaryOrder = mapConfig->referenceFrame.metaBinaryOrder;
    return TileId (nodeId.lod,
       (nodeId.x >> metaTileBinaryOrder) << metaTileBinaryOrder,
       (nodeId.y >> metaTileBinaryOrder) << metaTileBinaryOrder);
}

void MapImpl::touchDraws(const RenderTask &task)
{
    if (task.meshAgg)
        touchResource(task.meshAgg);
    if (task.textureColor)
        touchResource(task.textureColor);
    if (task.textureMask)
        touchResource(task.textureMask);
}

void MapImpl::touchDraws(const std::vector<RenderTask> &renders)
{
    for (auto &it : renders)
        touchDraws(it);
}

void MapImpl::touchDraws(TraverseNode *trav)
{
    for (auto &it : trav->opaque)
        touchDraws(it);
    for (auto &it : trav->transparent)
        touchDraws(it);
}

bool MapImpl::prerequisitesCheck()
{
    if (resources.auth)
        resources.auth->checkTime();

    if (initialized)
        return true;

    if (mapConfigPath.empty())
        return false;

    if (!resources.authPath.empty())
    {
        resources.auth = getAuthConfig(resources.authPath);
        if (!testAndThrow(resources.auth->state, "Authentication failure."))
            return false;
    }

    mapConfig = getMapConfig(mapConfigPath);
    if (!testAndThrow(mapConfig->state, "Map config failure."))
        return false;

    if (layers.empty())
        layers.push_back(std::make_shared<MapLayer>(this));

    for (auto &it : layers)
    {
        if (!it->prerequisitesCheck())
            return false;
    }

    renderer.credits.merge(mapConfig.get());
    initializeNavigation();
    mapConfig->initializeCelestialBody();

    LOG(info3) << "Map config ready";
    initialized = true;
    if (callbacks.mapconfigReady)
        callbacks.mapconfigReady(); // this may change initialized state
    return initialized;
}

void MapImpl::renderTickPrepare()
{
    if (!prerequisitesCheck())
        return;

    assert(!resources.auth || *resources.auth);
    assert(mapConfig && *mapConfig);
    assert(convertor);
    assert(!layers.empty());
    assert(layers[0]->traverseRoot);

    updateNavigation();
    updateSearch();
    updateSris();
    for (auto &it : layers)
        traverseClearing(it->traverseRoot.get());
}

void MapImpl::renderTickRender()
{
    draws.clear();

    if (!initialized || renderer.windowWidth == 0 || renderer.windowHeight == 0)
        return;

    updateCamera();
    for (auto &it : layers)
        traverseRender(it->traverseRoot.get());
    renderer.credits.tick(credits);
    for (const RenderTask &r : navigation.renders)
        draws.Infographic.emplace_back(r, this);

    draws.sortOpaqueFrontToBack();
}

} // namespace vts
