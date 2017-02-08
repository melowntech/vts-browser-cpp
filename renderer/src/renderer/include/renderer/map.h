#ifndef MAP_H_jihsefk
#define MAP_H_jihsefk

#include <string>
#include <memory>

#include "foundation.h"

namespace melown
{
    class MELOWN_API MapOptions
    {
    public:
        MapOptions();

        class GlContext *glRenderer;
        class GlContext *glData;
        class Fetcher *fetcher;
        uint64 cacheHddLimit;
        uint64 cacheRamLimit;
        uint64 gpuMemoryLimit;
    };

    namespace
    {
        class MapImpl;
    }

    class MELOWN_API Map
    {
    public:
        Map(const MapOptions &options);
        ~Map();

        void setMapConfig(const std::string &path);
        void setMapConfig(void *buffer, uint32 length);
        void dataInitialize();
        bool dataUpdate();
        void dataFinalize();
        void render();

    private:
        MapImpl *impl;
    };
}

#endif
