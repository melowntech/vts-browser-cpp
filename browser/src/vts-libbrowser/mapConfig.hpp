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

#ifndef MAPCONFIG_HPP_sdf45gde5g4
#define MAPCONFIG_HPP_sdf45gde5g4

#include <unordered_map>

#include <vts-libs/vts/mapconfig.hpp>

#include "resource.hpp"

namespace Json
{
    class Value;
}

namespace vtslibs { namespace vts
{
    class NodeInfo;
}} // namespace vtslibs::vts

namespace vts
{

class GpuAtmosphereDensityTexture;
class BoundInfo;
class FreeInfo;

class ExternalBoundLayer : public Resource,
    public vtslibs::registry::BoundLayer
{
public:
    ExternalBoundLayer(MapImpl *map, const std::string &name);
    void decode() override;
    FetchTask::ResourceType resourceType() const override;
};

class ExternalFreeLayer : public Resource,
    public vtslibs::registry::FreeLayer
{
public:
    ExternalFreeLayer(MapImpl *map, const std::string &name);
    void decode() override;
    FetchTask::ResourceType resourceType() const override;
};

class Mapconfig : public Resource, public vtslibs::vts::MapConfig
{
public:
    class BrowserOptions
    {
    public:
        std::shared_ptr<Json::Value> value;
        std::string searchUrl;
        std::string searchSrs;
        double autorotate = 0;
    };

    Mapconfig(MapImpl *map, const std::string &name);
    ~Mapconfig();
    void decode() override;
    FetchTask::ResourceType resourceType() const override;
    vtslibs::registry::Srs::Type navigationSrsType() const;

    void consolidateView();
    void initializeCelestialBody();
    bool isEarth() const;

    BoundInfo *getBoundInfo(const std::string &id);
    FreeInfo *getFreeInfo(const std::string &id);
    vtslibs::vts::SurfaceCommonConfig *findGlue(
        const vtslibs::vts::Glue::Id &id);
    vtslibs::vts::SurfaceCommonConfig *findSurface(
        const std::string &id);

    BrowserOptions browserOptions;
    std::shared_ptr<GpuAtmosphereDensityTexture> atmosphereDensityTexture;
    std::vector<vtslibs::vts::NodeInfo> referenceDivisionNodeInfos;

private:
    std::unordered_map<std::string, std::shared_ptr<BoundInfo>> boundInfos;
    std::unordered_map<std::string, std::shared_ptr<FreeInfo>> freeInfos;
};

} // namespace vts

#endif
