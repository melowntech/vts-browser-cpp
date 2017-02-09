#include <renderer/foundation.h>

#include "../../vts-libs/vts/mapconfig.hpp"

namespace melown
{
	extern vadstena::vts::MapConfig mapConfig;

	void parseMapConfig(const std::string &path, const char *buffer, uint32 size);
}
