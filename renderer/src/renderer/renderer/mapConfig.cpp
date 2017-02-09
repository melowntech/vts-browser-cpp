#include <string>
#include <sstream>
#include <iostream>

#include "mapConfig.h"

namespace melown
{
    vadstena::vts::MapConfig mapConfig;

    void parseMapConfig(const std::string &path, const char *buffer, uint32 size)
    {
        std::istringstream is(std::string(buffer, size));
        vadstena::vts::loadMapConfig(mapConfig, is, path);
    }
}
