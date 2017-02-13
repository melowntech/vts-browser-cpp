#ifndef GPUMANAGER_H_wqioeufhiwgb
#define GPUMANAGER_H_wqioeufhiwgb

#include <string>

namespace melown
{
    class GpuManager
    {
    public:
        static GpuManager *create(class Map *map);

        GpuManager();
        virtual ~GpuManager();

        virtual void dataInitialize(class GpuContext *context, class Fetcher *fetcher) = 0;
        virtual bool dataTick() = 0;
        virtual void dataFinalize() = 0;

        virtual void renderInitialize(class GpuContext *context) = 0;
        virtual void renderTick() = 0;
        virtual void renderFinalize() = 0;

        virtual class GpuShader *getShader(const std::string &name) = 0;
        virtual class GpuTexture *getTexture(const std::string &name) = 0;
        virtual class GpuSubMesh *getSubMesh(const std::string &name) = 0;
        virtual class GpuMeshAggregate *getMeshAggregate(const std::string &name) = 0;
    };
}

#endif
