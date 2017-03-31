#include <dbglog/dbglog.hpp>

#include <renderer/options.h>

namespace melown
{

MapOptions::MapOptions() :
    renderWireBoxes(false),
    renderSurrogates(false),
    navigationSamplesPerViewExtent(4),
    maxTexelToPixelScale(1.2),
    maxResourcesMemory(512*1024*1024),
    maxConcurrentDownloads(20)
{}

MapOptions::~MapOptions()
{}

} // namespace melown
