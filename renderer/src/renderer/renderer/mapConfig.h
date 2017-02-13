#ifndef MAPCONFIG_H_sdfkjwbhv
#define MAPCONFIG_H_sdfkjwbhv

#include <string>

#include "foundation.h"

#include "../../vts-libs/vts/mapconfig.hpp"

namespace melown
{
    using vadstena::vts::MapConfig;
    void parseMapConfig(MapConfig *mapConfig, const std::string &path, const char *buffer, uint32 size);
}

#endif
