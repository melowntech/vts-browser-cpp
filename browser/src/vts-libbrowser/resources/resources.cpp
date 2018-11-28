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

#include "../map.hpp"

namespace vts
{

namespace
{

template<class T>
std::shared_ptr<T> getMapResource(MapImpl *map, const std::string &name)
{
    assert(!name.empty());
    auto it = map->resources.resources.find(name);
    if (it == map->resources.resources.end())
    {
        auto r = std::make_shared<T>(map, name);
        map->resources.resources[name] = r;
        it = map->resources.resources.find(name);
        map->statistics.resourcesCreated++;
    }
    assert(it->second);
    map->touchResource(it->second);
    auto res = std::dynamic_pointer_cast<T>(it->second);
    assert(res);
    return res;
}

} // namespace

MapImpl::Resources::Resources() :
    downloads(0),  progressEstimationMaxResources(0)
{}

MapImpl::Resources::Fetching::Fetching() :
    stop(false)
{}

void MapImpl::touchResource(const std::shared_ptr<Resource> &resource)
{
    resource->lastAccessTick = renderTickIndex;
}

Validity MapImpl::getResourceValidity(const std::string &name)
{
    auto it = resources.resources.find(name);
    if (it == resources.resources.end())
        return Validity::Invalid;
    return getResourceValidity(it->second);
}

Validity MapImpl::getResourceValidity(
    const std::shared_ptr<Resource> &resource)
{
    switch ((Resource::State)resource->state)
    {
    case Resource::State::errorFatal:
    case Resource::State::availFail:
        return Validity::Invalid;
    case Resource::State::ready:
        return Validity::Valid;
    default:
        return Validity::Indeterminate;
    }
}

std::shared_ptr<GpuTexture> MapImpl::getTexture(const std::string &name)
{
    return getMapResource<GpuTexture>(this, name);
}

std::shared_ptr<GpuAtmosphereDensityTexture>
MapImpl::getAtmosphereDensityTexture(
    const std::string &name)
{
    return getMapResource<GpuAtmosphereDensityTexture>(this, name);
}

std::shared_ptr<GpuMesh> MapImpl::getMesh(const std::string &name)
{
    return getMapResource<GpuMesh>(this, name);
}

std::shared_ptr<AuthConfig> MapImpl::getAuthConfig(const std::string &name)
{
    return getMapResource<AuthConfig>(this, name);
}

std::shared_ptr<Mapconfig> MapImpl::getMapconfig(const std::string &name)
{
    return getMapResource<Mapconfig>(this, name);
}

std::shared_ptr<MetaTile> MapImpl::getMetaTile(const std::string &name)
{
    return getMapResource<MetaTile>(this, name);
}

std::shared_ptr<NavTile> MapImpl::getNavTile(
        const std::string &name)
{
    return getMapResource<NavTile>(this, name);
}

std::shared_ptr<MeshAggregate> MapImpl::getMeshAggregate(
        const std::string &name)
{
    return getMapResource<MeshAggregate>(this, name);
}

std::shared_ptr<ExternalBoundLayer> MapImpl::getExternalBoundLayer(
        const std::string &name)
{
    return getMapResource<ExternalBoundLayer>(this, name);
}

std::shared_ptr<ExternalFreeLayer> MapImpl::getExternalFreeLayer(
        const std::string &name)
{
    return getMapResource<ExternalFreeLayer>(this, name);
}

std::shared_ptr<BoundMetaTile> MapImpl::getBoundMetaTile(
        const std::string &name)
{
    return getMapResource<BoundMetaTile>(this, name);
}

std::shared_ptr<SearchTaskImpl> MapImpl::getSearchTask(const std::string &name)
{
    return getMapResource<SearchTaskImpl>(this, name);
}

std::shared_ptr<TilesetMapping> MapImpl::getTilesetMapping(
        const std::string &name)
{
    return getMapResource<TilesetMapping>(this, name);
}

std::shared_ptr<GeodataFeatures> MapImpl::getGeoFeatures(
        const std::string &name)
{
    return getMapResource<GeodataFeatures>(this, name);
}

std::shared_ptr<GeodataStylesheet> MapImpl::getGeoStyle(
        const std::string &name)
{
    return getMapResource<GeodataStylesheet>(this, name);
}

std::shared_ptr<GpuGeodata> MapImpl::getGeodata(const std::string &name)
{
    return getMapResource<GpuGeodata>(this, name);
}

} // namespace vts
