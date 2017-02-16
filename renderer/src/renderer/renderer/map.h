#ifndef MAP_H_jihsefk
#define MAP_H_jihsefk

#include <string>
#include <memory>

#include "foundation.h"

namespace melown
{
    class Map
    {
    public:
        Map(const std::string &mapConfigPath);
        virtual ~Map();

        void dataInitialize(class GpuContext *context, class Fetcher *fetcher);
        bool dataTick();
        void dataFinalize();

        void renderInitialize(class GpuContext *context);
        void renderTick(uint32 width, uint32 height);
        void renderFinalize();

        class Cache *cache;
        class ResourceManager *resources;
        class Renderer *renderer;

        std::string mapConfigPath;
    };
}

#endif
