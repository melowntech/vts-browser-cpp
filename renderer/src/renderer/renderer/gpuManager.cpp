#include <unordered_map>
#include <unordered_set>

#include "map.h"
#include "cache.h"
#include "gpuManager.h"
#include "gpuResources.h"
#include "gpuContext.h"

#include <boost/thread/mutex.hpp>

namespace melown
{
    class GpuManagerImpl : public GpuManager
    {
    public:
        /////////
        /// DATA thread
        /////////

        GpuManagerImpl(Map *map) : map(map), renderContext(nullptr), dataContext(nullptr)
        {}

        ~GpuManagerImpl()
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
                if (pending_data.empty())
                    return false;
                name = *pending_data.begin();
                pending_data.erase(pending_data.begin());
            }
            GpuResource *r = resources[name].get();
            if (r->ready)
                return true;
            r->loadToGpu(name, map);
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

        void touch(const std::string &name, GpuResource *resource)
        {
            pending_render.insert(name);
        }

        GpuShader *getShader(const std::string &name) override
        {
            auto it = resources.find(name);
            if (it == resources.end())
            {
                resources[name] = renderContext->createShader();
                it = resources.find(name);
            }
            touch(name, it->second.get());
            return dynamic_cast<GpuShader*>(it->second.get());
        }

        GpuTexture *getTexture(const std::string &name) override
        {
            auto it = resources.find(name);
            if (it == resources.end())
            {
                resources[name] = renderContext->createTexture();
                it = resources.find(name);
            }
            touch(name, it->second.get());
            return dynamic_cast<GpuTexture*>(it->second.get());
        }

        GpuSubMesh *getSubMesh(const std::string &name) override
        {
            auto it = resources.find(name);
            if (it == resources.end())
            {
                resources[name] = renderContext->createSubMesh();
                it = resources.find(name);
            }
            touch(name, it->second.get());
            return dynamic_cast<GpuSubMesh*>(it->second.get());
        }

        GpuMeshAggregate *getMeshAggregate(const std::string &name) override
        {
            auto it = resources.find(name);
            if (it == resources.end())
            {
                resources[name] = renderContext->createMeshAggregate();
                it = resources.find(name);
            }
            touch(name, it->second.get());
            return dynamic_cast<GpuMeshAggregate*>(it->second.get());
        }

        std::unordered_map<std::string, std::shared_ptr<GpuResource>> resources;
        std::unordered_set<std::string> pending_render;
        std::unordered_set<std::string> pending_data;
        boost::mutex mut;

        Map *map;
        GpuContext *renderContext;
        GpuContext *dataContext;
    };

    GpuManager::GpuManager()
    {}

    GpuManager::~GpuManager()
    {}

    GpuManager *GpuManager::create(Map *map)
    {
        return new GpuManagerImpl(map);
    }
}
