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

#include <utility/uri.hpp>

#include "map.hpp"

namespace vts
{

void MapImpl::renderInitialize()
{
    LOG(info3) << "Render initialize";
}

void MapImpl::renderFinalize()
{
    LOG(info3) << "Render finalize";
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

        credits.merge(mapconfig.get());
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

#ifdef VTS_ENABLE_FREE_LAYERS
        // free layers
        for (const auto &it : mapconfig->view.freeLayers)
            layers.push_back(std::make_shared<MapLayer>(this, it.first,
                it.second));
#endif
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

    LOG(info2) << "Mapconfig is ready.";
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
        trav->clearAll();
        return;
    }

    if (trav->lastRenderTime + 5 < renderTickIndex)
        trav->clearRenders();

    for (auto &it : trav->childs)
        traverseClearing(it.get());
}

double MapImpl::getMapRenderProgress()
{
    uint32 active = statistics.resourcesPreparing;
    if (active == 0)
    {
        resources.progressEstimationMaxResources = 0;
        return 0;
    }

    resources.progressEstimationMaxResources
            = std::max(resources.progressEstimationMaxResources, active);
    return double(resources.progressEstimationMaxResources - active)
            / resources.progressEstimationMaxResources;
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
    TileId id = trav->nodeInfo.nodeId();
    if (id == what)
        return trav;
    if (what.lod <= id.lod)
        return findTravById(trav->parent, what);
    TileId t = what;
    while (t.lod > id.lod + 1)
        t = vtslibs::vts::parent(t);
    for (auto &it : trav->childs)
        if (it->nodeInfo.nodeId() == t)
            return findTravById(it.get(), what);
    return nullptr;
}

} // namespace vts

