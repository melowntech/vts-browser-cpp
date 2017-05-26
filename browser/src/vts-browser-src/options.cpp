#include <dbglog/dbglog.hpp>
#include "include/vts-browser/options.hpp"

namespace vts
{

void setLogMask(const std::string &mask)
{
    dbglog::set_mask(mask);
}

void setLogConsole(bool enable)
{
    dbglog::log_console(enable);
}

void setLogFile(const std::string &filename)
{
    dbglog::log_file(filename);
}

MapOptions::MapOptions() :
    searchUrl("https://n1.windyty.com/search.php?q={query}&format=json"
              "&lang=en-US&addressdetails=1&limit=20"),
    maxTexelToPixelScale(1.2),
    positionViewExtentMin(75),
    positionViewExtentMax(1e7),
    cameraSensitivityPan(1),
    cameraSensitivityZoom(1),
    cameraSensitivityRotate(1),
    cameraInertiaPan(0.85),
    cameraInertiaZoom(0.85),
    cameraInertiaRotate(0.85),
    navigationLatitudeThreshold(80),
    maxResourcesMemory(512 * 1024 * 1024),
    maxConcurrentDownloads(10),
    maxNodeUpdatesPerTick(10),
    maxResourceProcessesPerTick(5),
    navigationSamplesPerViewExtent(4),
    navigationMode(NavigationMode::Dynamic),
    renderSurrogates(false),
    renderMeshBoxes(false),
    renderTileBoxes(false),
    renderObjectPosition(false),
    searchResultsFilter(true),
    debugDetachedCamera(false),
    debugDisableMeta5(false)
{}

MapOptions::~MapOptions()
{}

} // namespace vts
