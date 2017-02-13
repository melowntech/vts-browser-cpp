#ifndef MAP_H_jihsefk
#define MAP_H_jihsefk

#include <string>
#include <memory>

#include "foundation.h"

namespace vadstena
{
    namespace vts
    {
        class MapConfig;
    }
}

namespace melown
{
    using vadstena::vts::MapConfig;

    class Map
    {
    public:
        Map(const std::string &mapConfigPath);
        virtual ~Map();

        void dataInitialize(class GpuContext *context, class Fetcher *fetcher);
        bool dataTick();
        void dataFinalize();
        void renderInitialize(class GpuContext *context);
        void renderTick();
        void renderFinalize();

        class Cache *cache;
        class GpuManager *gpuManager;
        class Renderer *renderer;
        class MapConfig *mapConfig;

        std::string mapConfigPath;
    };
}

#endif
