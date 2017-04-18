#include <dbglog/dbglog.hpp>

#include <vts/options.hpp>

namespace vts
{

MapOptions::MapOptions() :
    renderWireBoxes(false),
    renderSurrogates(false),
    renderObjectPosition(false),
    debugDetachedCamera(false),
    renderTileCorners(false),
    debugDisableMeta5(false),
    navigationSamplesPerViewExtent(4),
    positionViewExtentMin(75),
    positionViewExtentMax(1e7),
    maxTexelToPixelScale(1.2),
    autoRotateSpeed(0),
    maxResourcesMemory(512*1024*1024),
    maxConcurrentDownloads(30),
    maxNodeUpdatesPerFrame(10)
{}

MapOptions::~MapOptions()
{}

} // namespace vts
