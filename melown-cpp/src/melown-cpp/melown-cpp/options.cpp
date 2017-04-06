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
    autoRotateSpeed(0),
    maxResourcesMemory(512*1024*1024),
    maxConcurrentDownloads(30),
    maxNodeUpdatesPerFrame(10)
{}

MapOptions::~MapOptions()
{}

} // namespace melown
