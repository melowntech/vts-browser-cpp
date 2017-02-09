#include <renderer/map.h>
#include <renderer/glContext.h>
#include <renderer/glClasses.h>
#include <renderer/fetcher.h>

#include "../../dbglog/dbglog.hpp"

#include "constants.h"
#include "cache.h"
#include "mapConfig.h"

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
            MapImpl(Map *map, const MapOptions &options) : map(map), glRenderer(nullptr), glData(nullptr), fetcher(nullptr), cache(options)
            {
                glRenderer = options.glRenderer;
                glData = options.glData;
                fetcher = options.fetcher;
                {
                    Fetcher::Func func = std::bind(&MapImpl::fetchedFile, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
                    fetcher->setCallback(func);
                }
            }

            void fetchedFile(FetchType type, const std::string &name, const char *buffer, uint32 size)
            {
                if (!buffer)
                {
                    LOG(err1) << "fetching file" << name << "failed";
                    return;
                }
                LOG(info1) << "fetched file" << name;
                switch (type)
                {
                case FetchType::MapConfig: return fetchedMapConfig(name, buffer, size);
                default: throw "invalid fetch type";
                }
            }

            void fetchedMapConfig(const std::string &path, const char *buffer, uint32 size)
            {
                cache.purgeAll();
                parseMapConfig(path, buffer, size);
                LOG(err1) << "map config" << path << "loaded";
            }

            void setMapConfig(const std::string &path)
            {
                fetcher->fetch(FetchType::MapConfig, path);
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

            Map *map;
            GlContext *glRenderer;
            GlContext *glData;
            Fetcher *fetcher;
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
