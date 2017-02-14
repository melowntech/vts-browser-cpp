#include "map.h"
#include "cache.h"
#include "mapResources.h"

namespace melown
{
    void MapConfig::load(const std::string &name, class Map *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        if (base->cache->read(name, buffer, size))
        {
            std::istringstream is(std::string((char*)buffer, size));
            vadstena::vts::loadMapConfig(*this, is, name);
            basePath = name.substr(0, name.find_last_of('/') + 1);
            ready = true;
        }
    }

    MetaTile::MetaTile() : vadstena::vts::MetaTile(vadstena::vts::TileId(), 0)
    {}

    void MetaTile::load(const std::string &name, class Map *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        if (base->cache->read(name, buffer, size))
        {
            std::istringstream is(std::string((char*)buffer, size));
            *(vadstena::vts::MetaTile*)this = vadstena::vts::loadMetaTile(is, 5, name);
            ready = true;
        }
    }
}
