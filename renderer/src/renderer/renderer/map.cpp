#include <renderer/map.h>
#include <renderer/glContext.h>
#include <renderer/glClasses.h>

#include "cache.h"

#include <stdio.h>

namespace melown
{
    MapOptions::MapOptions() : glRenderer(nullptr), glData(nullptr), fetcher(nullptr),
        cacheHddLimit(10000000000), cacheRamLimit(2000000000), gpuMemoryLimit(500000000)
    {}

    namespace
    {
        class MapImpl
        {
        public:
            MapImpl(Map *map, const MapOptions &options) : glRenderer(nullptr), glData(nullptr), cache(options)
            {
                glRenderer = options.glRenderer;
                glData = options.glData;
            }

            void setMapConfig(const std::string &path)
            {

            }

            void setMapConfig(void *buffer, uint32 length)
            {

            }

            void dataInitialize()
            {
            }

            bool dataUpdate()
            {
                return false;
            }

            void dataFinalize()
            {
            }

            void render()
            {
            }

            GlContext *glRenderer;
            GlContext *glData;
            Cache cache;
        };
    }

    Map::Map(const MapOptions &options) : impl(nullptr)
    {
        this->impl = new MapImpl(this, options);
    }

    Map::~Map()
    {
        delete this->impl;
    }

    void Map::setMapConfig(const std::string &path)
    {
        impl->setMapConfig(path);
    }

    void Map::setMapConfig(void *buffer, uint32 length)
    {
        impl->setMapConfig(buffer, length);
    }

    void Map::dataInitialize()
    {
        impl->dataInitialize();
    }

    bool Map::dataUpdate()
    {
        return impl->dataUpdate();
    }

    void Map::dataFinalize()
    {
        impl->dataFinalize();
    }

    void Map::render()
    {
        impl->render();
    }
}
