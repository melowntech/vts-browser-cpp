#include <string>
#include <sstream>
#include <iostream>

#include "mapConfig.h"

namespace melown
{
    void parseMapConfig(MapConfig *mapConfig, const std::string &path, const char *buffer, uint32 size)
    {
        std::istringstream is(std::string(buffer, size));
        vadstena::vts::loadMapConfig(*mapConfig, is, path);
    }
}
