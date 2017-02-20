#include "map.h"
#include "cache.h"
#include "mapConfig.h"

namespace melown
{
    MapConfig::MapConfig(const std::string &name) : Resource(name)
    {}

    void MapConfig::load(MapImpl *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        switch (base->cache->read(name, buffer, size))
        {
        case Cache::Result::ready:
        {
            std::istringstream is(std::string((char*)buffer, size));
            vadstena::vts::loadMapConfig(*this, is, name);
            basePath = name.substr(0, name.find_last_of('/') + 1);
            state = State::ready;
            return;
        }
        case Cache::Result::error:
            state = State::errorDownload;
            return;
        }
    }
}
