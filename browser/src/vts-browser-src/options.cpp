/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
    searchUrl("https://n1.windyty.com/search.php?format=json"
              "&addressdetails=1&limit=20&q={query}"),
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
    navigationSamplesPerViewExtent(8),
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
