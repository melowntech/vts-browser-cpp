#include <unordered_map>
#include <unordered_set>

#include "map.h"
#include "cache.h"
#include "gpuContext.h"
#include "gpuResources.h"
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

        ResourceManagerImpl(Map *map) : map(map), takeItemIndex(0)
        {}

        ~ResourceManagerImpl()
        {}

        void dataInitialize(GpuContext *context, Fetcher *fetcher) override
        {
            dataContext = context;
            map->cache = Cache::create(map, fetcher);
        }

        bool dataTick() override
        {
            std::string name;
            { // take an item
                boost::lock_guard<boost::mutex> l(mut);
                if (!pending_render.empty())
                {
                    std::swap(pending_render, pending_data);
                    pending_render.clear();
                }
                if (pending_data.empty())
                    return false;
                auto it = std::next(pending_data.begin(), takeItemIndex++ % pending_data.size());
                name = *it;
                pending_data.erase(it);
            }
            Resource *r = resources[name].get();
            if (r->ready)
                return true;
            r->load(name, map);
            return r->ready;
        }

        void dataFinalize() override
        {
            dataContext = nullptr;
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
                pending_data.insert(pending_render.begin(), pending_render.end());
            }
            pending_render.clear();
        }

        void renderFinalize() override
        {
            renderContext = nullptr;
        }

        void touch(const std::string &name, Resource *resource)
        {
            if (!resource->ready)
                pending_render.insert(name);
        }

        template<class T, std::shared_ptr<Resource>(GpuContext::*F)()> T *getGpuResource(const std::string &name)
        {
            auto it = resources.find(name);
            if (it == resources.end())
            {
                resources[name] = (renderContext->*F)();
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
            return getGpuResource<GpuMeshRenderable, &GpuContext::createMeshRenderable>(name);
        }

        template<class T> T *getMapResource(const std::string &name)
        {
            auto it = resources.find(name);
            if (it == resources.end())
            {
                resources[name] = std::shared_ptr<Resource>(new T());
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

        std::unordered_map<std::string, std::shared_ptr<Resource>> resources;
        std::unordered_set<std::string> pending_render;
        std::unordered_set<std::string> pending_data;
        boost::mutex mut;

        Map *map;
        uint32 takeItemIndex;
    };

    ResourceManager::ResourceManager() : renderContext(nullptr), dataContext(nullptr)
    {}

    ResourceManager::~ResourceManager()
    {}

    ResourceManager *ResourceManager::create(Map *map)
    {
        return new ResourceManagerImpl(map);
    }
}
