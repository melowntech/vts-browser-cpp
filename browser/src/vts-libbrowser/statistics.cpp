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

#include "include/vts-browser/statistics.hpp"
#include "include/vts-browser/options.hpp"

namespace vts
{

MapStatistics::MapStatistics()
{
    resetAll();
}

MapStatistics::~MapStatistics()
{}

void MapStatistics::resetAll()
{
    resetFrame();
    resourcesDownloaded = 0;
    resourcesDiskLoaded = 0;
    resourcesProcessed = 0;
    resourcesCreated = 0;
    resourcesReleased = 0;
    resourcesFailed = 0;
    renderTicks = 0;
    dataTicks = 0;
    resourcesDownloading = 0;
    currentGpuMemUse = 0;
    currentRamMemUse = 0;
    resourcesActive = 0;
    resourcesPreparing = 0;
    currentNavigationMode = (NavigationMode)0;
}

void MapStatistics::resetFrame()
{
    currentNodeMetaUpdates = 0;
    currentNodeDrawsUpdates = 0;
    meshesRenderedTotal = 0;
    metaNodesTraversedTotal = 0;
    for (uint32 i = 0; i < MaxLods; i++)
    {
        meshesRenderedPerLod[i] = 0;
        metaNodesTraversedPerLod[i] = 0;
    }
}

} // namespace vts
