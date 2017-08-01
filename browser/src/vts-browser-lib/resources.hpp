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

#ifndef RESOURCES_H_ergiuhdusgju
#define RESOURCES_H_ergiuhdusgju

#include <memory>
#include <string>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <vts-libs/vts/mapconfig.hpp>
#include <vts-libs/vts/urltemplate.hpp>
#include <vts-libs/vts/metatile.hpp>
#include <vts-libs/vts/tsmap.hpp>
#include <boost/optional.hpp>

#include "include/vts-browser/resources.hpp"
#include "include/vts-browser/math.hpp"
#include "include/vts-browser/fetcher.hpp"

#ifndef NDEBUG
    #define unordered_map map
#endif

namespace vts
{

class MapImpl;

class Resource : public FetchTask
{
public:
    enum class State
    {
        initializing,
        downloading,
        downloaded,
        ready,
        errorFatal,
        errorRetry,
        availFail,
        finalizing,
    };

    Resource(MapImpl *map, const std::string &name,
             FetchTask::ResourceType resourceType);
    virtual ~Resource();
    virtual void load() = 0;
    void fetchDone() override;
    void processLoad(); // calls load
    bool performAvailTest() const;
    void updatePriority(float priority);
    operator bool () const;

    ResourceInfo info;
    const std::string name;
    MapImpl *const map;
    boost::optional<vtslibs::registry::BoundLayer::Availability> availTest;
    std::atomic<State> state;
    std::time_t retryTime;
    uint32 retryNumber;
    uint32 redirectionsCount;
    uint32 lastAccessTick;
    float priority;
    float priorityCopy;
};

class GpuMesh : public Resource
{
public:
    GpuMesh(MapImpl *map, const std::string &name);
    void load() override;
};

class GpuTexture : public Resource
{
public:
    GpuTexture(MapImpl *map, const std::string &name);
    void load() override;
};

class AuthConfig : public Resource
{
public:
    AuthConfig(MapImpl *map, const std::string &name);
    void load() override;
    void checkTime();
    void authorize(Resource *task);

private:
    std::string token;
    std::unordered_set<std::string> hostnames;
    uint64 timeValid;
    uint64 timeParsed;
};

class ExternalBoundLayer : public Resource,
        public vtslibs::registry::BoundLayer
{
public:
    ExternalBoundLayer(MapImpl *map, const std::string &name);
    void load() override;
};

class MapConfig : public Resource, public vtslibs::vts::MapConfig
{
public:
    class BoundInfo : public vtslibs::registry::BoundLayer
    {
    public:
        BoundInfo(const vtslibs::registry::BoundLayer &bl,
                  const std::string &url);
    
        vtslibs::vts::UrlTemplate urlExtTex;
        vtslibs::vts::UrlTemplate urlMeta;
        vtslibs::vts::UrlTemplate urlMask;
    };
    
    class SurfaceInfo : public vtslibs::vts::SurfaceCommonConfig
    {
    public:
        SurfaceInfo(const SurfaceCommonConfig &surface,
                    const std::string &parentPath);
    
        vtslibs::vts::UrlTemplate urlMeta;
        vtslibs::vts::UrlTemplate urlMesh;
        vtslibs::vts::UrlTemplate urlIntTex;
        vtslibs::vts::UrlTemplate urlNav;
        vtslibs::vts::TilesetIdList name;
    };
    
    class SurfaceStackItem
    {
    public:
        SurfaceStackItem();
        
        std::shared_ptr<SurfaceInfo> surface;
        vec3f color;
        bool alien;
    };
    
    class BrowserOptions
    {
    public:
        BrowserOptions();
        
        std::string searchUrl;
        std::string searchSrs;
        double autorotate;
    };
    
    MapConfig(MapImpl *map, const std::string &name);
    void load() override;
    void clear();
    static const std::string convertPath(const std::string &path,
                                         const std::string &parent);
    vtslibs::registry::Srs::Type navigationType() const;
    
    vtslibs::vts::SurfaceCommonConfig *findGlue(
            const vtslibs::vts::Glue::Id &id);
    vtslibs::vts::SurfaceCommonConfig *findSurface(const std::string &id);
    BoundInfo *getBoundInfo(const std::string &id);
    void printSurfaceStack();
    void generateSurfaceStack(const vtslibs::vts::VirtualSurfaceConfig
                              *virtualSurface = nullptr);
    static void colorizeSurfaceStack(std::vector<MapConfig::SurfaceStackItem>
                                     &surfaceStack);
    void consolidateView();
    void initializeCelestialBody() const;
    bool isEarth() const;
    
    std::unordered_map<std::string, std::shared_ptr<SurfaceInfo>> surfaceInfos;
    std::unordered_map<std::string, std::shared_ptr<BoundInfo>> boundInfos;
    std::vector<SurfaceStackItem> surfaceStack;
    BrowserOptions browserOptions;
};

class BoundMetaTile : public Resource
{
public:
    BoundMetaTile(MapImpl *map, const std::string &name);
    void load() override;

    uint8 flags[vtslibs::registry::BoundLayer::rasterMetatileWidth
                * vtslibs::registry::BoundLayer::rasterMetatileHeight];
};

class BoundMaskTile : public Resource
{
public:
    BoundMaskTile(MapImpl *map, const std::string &name);
    void load() override;

    std::shared_ptr<GpuTexture> texture;
};

class MetaTile : public Resource, public vtslibs::vts::MetaTile
{
public:
    MetaTile(MapImpl *map, const std::string &name);
    void load() override;
};

class MeshPart
{
public:
    MeshPart();
    std::shared_ptr<GpuMesh> renderable;
    mat4 normToPhys;
    uint32 textureLayer;
    uint32 surfaceReference;
    bool internalUv;
    bool externalUv;
};

class MeshAggregate : public Resource
{
public:
    MeshAggregate(MapImpl *map, const std::string &name);
    void load() override;

    std::vector<MeshPart> submeshes;
};

class NavTile : public Resource
{
public:
    NavTile(MapImpl *map, const std::string &name);
    void load() override;
    
    std::vector<unsigned char> data;
    
    static vec2 sds2px(const vec2 &point, const math::Extents2 &extents);
};

class SearchTaskImpl : public Resource
{
public:
    SearchTaskImpl(MapImpl *map, const std::string &name);
    void load() override;
    
    Buffer data;
    const std::string validityUrl;
    const std::string validitySrs;
};

class TilesetMapping : public Resource
{
public:
    TilesetMapping(MapImpl *map, const std::string &name);
    void load() override;
    void update(const std::vector<std::string> &vsId);

    vtslibs::vts::TilesetReferencesList dataRaw;
    std::vector<MapConfig::SurfaceStackItem> surfaceStack;
};

class SriIndex : public Resource
{
public:
    SriIndex(MapImpl *map, const std::string &name);
    void load() override;
    void update();
    
    std::vector<std::shared_ptr<MetaTile>> metatiles;
};

} // namespace vts

#endif
