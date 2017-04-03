#include <dbglog/dbglog.hpp>

#include <melown/options.hpp>

namespace melown
{

MapOptions::MapOptions() :
    renderWireBoxes(false),
    renderSurrogates(false),
    renderObjectPosition(false),
    navigationSamplesPerViewExtent(4),
    maxTexelToPixelScale(1.2),
    maxResourcesMemory(512*1024*1024),
    maxConcurrentDownloads(20)
{}

MapOptions::~MapOptions()
{}

} // namespace melown
