#include <unordered_map>
#include <unordered_set>

#include <boost/thread/mutex.hpp>

#include <renderer/gpuContext.h>
#include <renderer/gpuResources.h>
#include <renderer/fetcher.h>

#include "map.h"
#include "cache.h"
#include "mapConfig.h"
#include "mapResources.h"
#include "resourceManager.h"

namespace melown
{

class ResourceManagerImpl : public ResourceManager
{
public:
    /////////
    /// DATA thread
    /////////

    ResourceManagerImpl(MapImpl *map) : map(map), takeItemIndex(0), tickIndex(0)
    {}

    ~ResourceManagerImpl()
    {}

    void dataInitialize(GpuContext *context, Fetcher *fetcher) override
    {
        dataContext = context;
        map->cache = std::shared_ptr<Cache>(Cache::create(map, fetcher));
    }

    bool dataTick() override
    {
        Resource *res = nullptr;
        bool empty = true;
        { // take an item
            boost::lock_guard<boost::mutex> l(mut);
            if (!pending_data.empty())
            {
                auto it = std::next(pending_data.begin(),
                                    takeItemIndex++ % pending_data.size());
                res = *it;
                pending_data.erase(it);
                empty = pending_data.empty();
            }
        }
        if (!res)
        {
            map->cache->tick();
            return true; // all is done (sleep)
        }
        if (res->state != Resource::State::initializing)
            return false; // already loaded (try next)
        try
        {
            res->load(map);
        }
        catch (std::runtime_error &)
        {
            res->state = Resource::State::errorLoad;
        }
        return empty;
    }

    void dataFinalize() override
    {
        dataContext = nullptr;
        map->cache.reset();
        pending_data.clear();
    }

    /////////
    /// RENDER thread
    /////////

    void renderInitialize(GpuContext *context) override
    {
        renderContext = context;
    }

    void renderTick() override
    {
        // sync pending
        {
            boost::lock_guard<boost::mutex> l(mut);
            std::swap(pending_data, pending_render);
        }
        pending_render.clear();
        // clear old resources
        if (tickIndex % 10 == 0)
        {
            std::vector<Resource*> res;
            res.reserve(resources.size());
            uint32 memUse = 0;
            for (auto &&it : resources)
            {
                memUse += it.second->gpuMemoryCost + it.second->ramMemoryCost;
                // consider long time not used resources only
                if (it.second->lastAccessTick + 100 < tickIndex)
                    res.push_back(it.second.get());
            }
            static const uint32 memCap = 500000000;
            if (memUse > memCap)
            {
                std::sort(res.begin(), res.end(), [](Resource *a, Resource *b){
                    if (a->lastAccessTick == b->lastAccessTick)
                        return a->gpuMemoryCost + a->ramMemoryCost
                                > b->gpuMemoryCost + b->ramMemoryCost;
                    return a->lastAccessTick < b->lastAccessTick;
                });
                for (Resource *it : res)
                {
                    if (memUse <= memCap)
                        break;
                    memUse -= it->gpuMemoryCost + it->ramMemoryCost;
                    if (it->state != Resource::State::finalizing)
                        it->state = Resource::State::finalizing;
                    else
                        resources.erase(it->name);
                }
            }
        }
        tickIndex++;
    }

    void renderFinalize() override
    {
        renderContext = nullptr;
        pending_render.clear();
        resources.clear();
    }

    void touch(const std::string &name, Resource *resource)
    {
        resource->lastAccessTick = tickIndex;
        switch (resource->state)
        {
        case Resource::State::finalizing:
            resource->state = Resource::State::initializing;
            // intentionally no break here
        case Resource::State::initializing:
            pending_render.insert(resource);
            break;
        }
    }

    template<class T,
             std::shared_ptr<Resource>(GpuContext::*F)(const std::string &)>
    T *getGpuResource(const std::string &name)
    {
        auto it = resources.find(name);
        if (it == resources.end())
        {
            resources[name] = (renderContext->*F)(name);
            it = resources.find(name);
        }
        touch(name, it->second.get());
        return dynamic_cast<T*>(it->second.get());
    }

    GpuShader *getShader(const std::string &name) override
    {
        return getGpuResource<GpuShader, &GpuContext::createShader>(name);
    }

    GpuTexture *getTexture(const std::string &name) override
    {
        return getGpuResource<GpuTexture, &GpuContext::createTexture>(name);
    }

    GpuMeshRenderable *getMeshRenderable(const std::string &name) override
    {
        return getGpuResource<GpuMeshRenderable,
                &GpuContext::createMeshRenderable>(name);
    }

    template<class T> T *getMapResource(const std::string &name)
    {
        auto it = resources.find(name);
        if (it == resources.end())
        {
            resources[name] = std::shared_ptr<Resource>(new T(name));
            it = resources.find(name);
        }
        touch(name, it->second.get());
        return dynamic_cast<T*>(it->second.get());
    }

    MapConfig *getMapConfig(const std::string &name) override
    {
        return getMapResource<MapConfig>(name);
    }

    MetaTile *getMetaTile(const std::string &name) override
    {
        return getMapResource<MetaTile>(name);
    }

    MeshAggregate *getMeshAggregate(const std::string &name) override
    {
        return getMapResource<MeshAggregate>(name);
    }

    ExternalBoundLayer *getExternalBoundLayer(const std::string &name) override
    {
        return getMapResource<ExternalBoundLayer>(name);
    }

    BoundMetaTile *getBoundMetaTile(const std::string &name) override
    {
        return getMapResource<BoundMetaTile>(name);
    }

    BoundMaskTile *getBoundMaskTile(const std::string &name) override
    {
        return getMapResource<BoundMaskTile>(name);
    }
    
    bool ready(const std::string &name) override
    {
        auto &&it = resources.find(name);
        if (it == resources.end())
            return false;
        return it->second->state == Resource::State::ready;
    }

    std::unordered_map<std::string, std::shared_ptr<Resource>> resources;
    std::unordered_set<Resource*> pending_render;
    std::unordered_set<Resource*> pending_data;
    boost::mutex mut;

    MapImpl *map;
    uint32 takeItemIndex;
    uint32 tickIndex;
};

ResourceManager::ResourceManager() : renderContext(nullptr),
    dataContext(nullptr)
{}

ResourceManager::~ResourceManager()
{}

ResourceManager *ResourceManager::create(MapImpl *map)
{
    return new ResourceManagerImpl(map);
}

} // namespace melown
