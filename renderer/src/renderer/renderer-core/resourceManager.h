#ifndef RESOURCEMANAGER_H_wqioeufhiwgb
#define RESOURCEMANAGER_H_wqioeufhiwgb

#include <string>

namespace melown
{

class GpuContext;

class ResourceManager
{
public:
    static ResourceManager *create(class MapImpl *map);

    ResourceManager();
    virtual ~ResourceManager();

    virtual void dataInitialize(GpuContext *context,
                                class Fetcher *fetcher) = 0;
    virtual bool dataTick() = 0;
    virtual void dataFinalize() = 0;

    virtual void renderInitialize(GpuContext *context) = 0;
    virtual void renderTick() = 0;
    virtual void renderFinalize() = 0;

    virtual class GpuShader *getShader(const std::string &name) = 0;
    virtual class GpuTexture *getTexture(const std::string &name) = 0;
    virtual class GpuMeshRenderable *getMeshRenderable
                    (const std::string &name) = 0;

    virtual class MeshAggregate *getMeshAggregate(const std::string &name) = 0;
    virtual class MapConfig *getMapConfig(const std::string &name) = 0;
    virtual class MetaTile *getMetaTile(const std::string &name) = 0;
    virtual class NavTile *getNavTile(const std::string &name) = 0;
    virtual class ExternalBoundLayer *getExternalBoundLayer
                    (const std::string &name) = 0;
    virtual class BoundMetaTile *getBoundMetaTile(const std::string &name) = 0;
    virtual class BoundMaskTile *getBoundMaskTile(const std::string &name) = 0;

    virtual bool ready(const std::string &name) = 0;

    GpuContext *renderContext;
    GpuContext *dataContext;
};

} // namespace melown

#endif
