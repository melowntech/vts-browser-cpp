#include <unordered_map>
#include <unordered_set>

#include <renderer/gpuContext.h>
#include <renderer/gpuResources.h>
#include <renderer/fetcher.h>

#include "map.h"
#include "cache.h"
#include "mapConfig.h"
#include "mapResources.h"
#include "resourceManager.h"

#include <boost/thread/mutex.hpp>

namespace melown
{

class ResourceManagerImpl : public ResourceManager
{
public:
    /////////
    /// DATA thread
    /////////

    ResourceManagerImpl(MapImpl *map) : map(map), takeItemIndex(0)
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
        bool empty = false;
        { // take an item
            boost::lock_guard<boost::mutex> l(mut);
            if (pending_data.empty())
                return true; // all done
            auto it = std::next(pending_data.begin(),
                                takeItemIndex++ % pending_data.size());
            res = *it;
            pending_data.erase(it);
            empty = pending_data.empty();
        }
        if (res->state != Resource::State::initializing)
            return false; // immediately try next
        res->load(map);
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
        { // sync pending
            boost::lock_guard<boost::mutex> l(mut);
            std::swap(pending_data, pending_render);
        }
        pending_render.clear();
    }

    void renderFinalize() override
    {
        renderContext = nullptr;
        pending_render.clear();
        resources.clear();
    }

    void touch(const std::string &name, Resource *resource)
    {
        if (resource->state == Resource::State::initializing)
            pending_render.insert(resource);
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

    ExternalBoundLayer *getExternalBoundLayer(const std::string &name)
    {
        return getMapResource<ExternalBoundLayer>(name);
    }

    std::unordered_map<std::string, std::shared_ptr<Resource>> resources;
    std::unordered_set<Resource*> pending_render;
    std::unordered_set<Resource*> pending_data;
    boost::mutex mut;

    MapImpl *map;
    uint32 takeItemIndex;
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
