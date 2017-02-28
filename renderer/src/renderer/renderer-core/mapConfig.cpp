#include "map.h"
#include "cache.h"
#include "mapConfig.h"

namespace melown
{
    MapConfig::MapConfig(const std::string &name) : Resource(name)
    {}

    void MapConfig::load(MapImpl *base)
    {
        Buffer buffer;
        switch (base->cache->read(name, buffer))
        {
        case Cache::Result::ready:
        {
            std::istringstream is(std::string((char*)buffer.data, buffer.size));
            is.imbue(std::locale::classic());
            vadstena::vts::loadMapConfig(*this, is, name);
            state = State::ready;
            return;
        }
        case Cache::Result::error:
            state = State::errorDownload;
            return;
        }
    }
}
