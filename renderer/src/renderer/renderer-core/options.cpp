#include <dbglog/dbglog.hpp>

#include <renderer/options.h>

namespace melown
{

MapOptions::MapOptions() : renderWireBoxes(false),
    maxTexelToPixelScale(1.2),
    maxResourcesMemory(512*1024*1024),
    maxConcurrentDownloads(100)
{}

MapOptions::~MapOptions()
{}

} // namespace melown
