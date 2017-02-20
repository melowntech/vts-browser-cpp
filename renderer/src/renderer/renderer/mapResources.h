#ifndef MAPRESOURCES_H_hdsgfhjuwebgfj
#define MAPRESOURCES_H_hdsgfhjuwebgfj

#include <memory>
#include <string>

#include "../../vts-libs/vts/metatile.hpp"

#include <renderer/foundation.h>
#include <renderer/resource.h>

namespace melown
{
    class MetaTile : public Resource, public vadstena::vts::MetaTile
    {
    public:
        MetaTile(const std::string &name);
        void load(class MapImpl *base) override;
    };

    class MeshAggregate : public Resource
    {
    public:
        MeshAggregate(const std::string &name);

        void load(class MapImpl *base) override;

        std::vector<std::shared_ptr<class GpuMeshRenderable>> submeshes;
    };
}

#endif
