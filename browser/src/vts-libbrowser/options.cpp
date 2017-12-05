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

#include "include/vts-browser/options.hpp"
#include "include/vts-browser/map.hpp"

namespace vts
{

MapCreateOptions::MapCreateOptions() :
    clientId("undefined-vts-browser-cpp"),
    searchUrlFallback("https://n1.windyty.com/search.php?format=json"
                       "&addressdetails=1&limit=20&q={value}"),
    searchSrsFallback("+proj=longlat +datum=WGS84 +nodefs"),
    disableCache(false),
    hashCachePaths(true),
    disableSearchUrlFallbackOutsideEarth(true),
    disableBrowserOptionsSearchUrls(false)
{}

MapOptions::MapOptions() :
    maxTexelToPixelScale(1.2),
    maxTexelToPixelScaleBalancedAddition(3),
    viewExtentLimitScaleMin(0.00001175917), // 75 metres on earth
    viewExtentLimitScaleMax(2.35183443086), // 1.5e7 metres on earth
    viewExtentThresholdScaleLow(0.03135779241), // 200000 metres on earth
    viewExtentThresholdScaleHigh(0.20382565067), // 1300000 metres on earth
    tiltLimitAngleLow(270),
    tiltLimitAngleHigh(350),
    cameraSensitivityPan(1),
    cameraSensitivityZoom(1),
    cameraSensitivityRotate(1),
    cameraInertiaPan(0.9),
    cameraInertiaZoom(0.9),
    cameraInertiaRotate(0.9),
    cameraAltitudeFadeOutFactor(0.2),
    navigationLatitudeThreshold(80),
    navigationMaxViewExtentMult(1.02),
    navigationMaxPositionChange(0.02),
    renderTilesScale(1.001),
    targetResourcesMemory(0),
    maxConcurrentDownloads(10),
    maxResourceProcessesPerTick(10),
    navigationSamplesPerViewExtent(8),
    maxFetchRedirections(5),
    maxFetchRetries(5),
    fetchFirstRetryTimeOffset(1),
    navigationType(NavigationType::Quick),
    navigationMode(NavigationMode::Seamless),
    traverseMode(TraverseMode::Balanced),
    enableSearchResultsFilter(true),
    enableRuntimeResourceExpiration(false),
    enableArbitrarySriRequests(true),
    enableCameraNormalization(true),
    debugDetachedCamera(false),
    debugDisableVirtualSurfaces(false),
    debugDisableSri(true),
    debugSaveCorruptedFiles(true),
    debugFlatShading(false),
    debugRenderSurrogates(false),
    debugRenderMeshBoxes(false),
    debugRenderTileBoxes(false),
    debugRenderObjectPosition(false),
    debugRenderTargetPosition(false),
    debugRenderAltitudeShiftCorners(false),
    debugRenderNoMeshes(false)
{}

} // namespace vts
