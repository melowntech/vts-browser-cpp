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

#include "utilities/json.hpp"
#include "include/vts-browser/buffer.hpp"
#include "include/vts-browser/options.hpp"
#include "include/vts-browser/map.hpp"

namespace vts
{

MapCreateOptions::MapCreateOptions() :
    clientId("undefined-vts-browser-cpp"),
    searchUrlFallback("https://eu-n1.windyty.com/search.php?format=json"
                       "&addressdetails=1&limit=20&q={value}"),
    searchSrsFallback("+proj=longlat +datum=WGS84 +nodefs"),
    disableCache(false),
    hashCachePaths(true),
    disableSearchUrlFallbackOutsideEarth(true),
    disableBrowserOptionsSearchUrls(false),
    disableAtmosphereDensityTexture(false)
{}

MapCreateOptions::MapCreateOptions(const std::string &json)
    : MapCreateOptions()
{
    if (!json.empty())
        applyJson(json);
}

void MapCreateOptions::applyJson(const std::string &json)
{
    Json::Value v = stringToJson(json);
    AJ(clientId, asString);
    AJ(cachePath, asString);
    AJ(searchUrlFallback, asString);
    AJ(searchSrsFallback, asString);
    AJ(customSrs1, asString);
    AJ(customSrs2, asString);
    AJ(disableCache, asBool);
    AJ(hashCachePaths, asBool);
    AJ(disableSearchUrlFallbackOutsideEarth, asBool);
    AJ(disableBrowserOptionsSearchUrls, asBool);
}

std::string MapCreateOptions::toJson() const
{
    Json::Value v;
    TJ(clientId, asString);
    TJ(cachePath, asString);
    TJ(searchUrlFallback, asString);
    TJ(searchSrsFallback, asString);
    TJ(customSrs1, asString);
    TJ(customSrs2, asString);
    TJ(disableCache, asBool);
    TJ(hashCachePaths, asBool);
    TJ(disableSearchUrlFallbackOutsideEarth, asBool);
    TJ(disableBrowserOptionsSearchUrls, asBool);
    return jsonToString(v);
}

ControlOptions::ControlOptions() :
    sensitivityPan(1),
    sensitivityZoom(1),
    sensitivityRotate(1),
    inertiaPan(0.9),
    inertiaZoom(0.9),
    inertiaRotate(0.9)
{}

ControlOptions::ControlOptions(const std::string &json)
    : ControlOptions()
{
    if (!json.empty())
        applyJson(json);
}

void ControlOptions::applyJson(const std::string &json)
{
    Json::Value v = stringToJson(json);
    AJ(sensitivityPan, asDouble);
    AJ(sensitivityZoom, asDouble);
    AJ(sensitivityRotate, asDouble);
    AJ(inertiaPan, asDouble);
    AJ(inertiaZoom, asDouble);
    AJ(inertiaRotate, asDouble);
}

std::string ControlOptions::toJson() const
{
    Json::Value v;
    TJ(sensitivityPan, asDouble);
    TJ(sensitivityZoom, asDouble);
    TJ(sensitivityRotate, asDouble);
    TJ(inertiaPan, asDouble);
    TJ(inertiaZoom, asDouble);
    TJ(inertiaRotate, asDouble);
    return jsonToString(v);
}

MapOptions::MapOptions() :
    maxTexelToPixelScale(1.2),
    viewExtentLimitScaleMin(0.00001175917), // 75 metres on earth
    viewExtentLimitScaleMax(2.35183443086), // 1.5e7 metres on earth
    viewExtentThresholdScaleLow(0.03135779241), // 200 000 metres on earth
    viewExtentThresholdScaleHigh(0.20382565067), // 1 300 000 metres on earth
    tiltLimitAngleLow(-90),
    tiltLimitAngleHigh(-10),
    cameraAltitudeFadeOutFactor(0.5),
    navigationLatitudeThreshold(80),
    navigationPihaViewExtentMult(1.02),
    navigationPihaPositionChange(0.02),
    renderTilesScale(1.001),
    fixedTraversalDistance(10000),
    targetResourcesMemoryKB(0),
    maxConcurrentDownloads(25),
    maxResourceProcessesPerTick(10),
    navigationSamplesPerViewExtent(8),
    maxFetchRedirections(5),
    maxFetchRetries(5),
    fetchFirstRetryTimeOffset(1),
    fixedTraversalLod(15),
    navigationType(NavigationType::Quick),
    navigationMode(NavigationMode::Seamless),
    traverseModeSurfaces(TraverseMode::Balanced),
    traverseModeGeodata(TraverseMode::Hierarchical),
    enableSearchResultsFilter(true),
    enableArbitrarySriRequests(true),
    enableCameraNormalization(true),
    enableCameraAltitudeChanges(true),
    debugDetachedCamera(false),
    debugEnableVirtualSurfaces(true),
    debugEnableSri(false),
    debugSaveCorruptedFiles(false),
    debugFlatShading(false),
    debugValidateGeodataStyles(true),
    debugRenderSurrogates(false),
    debugRenderMeshBoxes(false),
    debugRenderTileBoxes(false),
    debugRenderObjectPosition(false),
    debugRenderTargetPosition(false),
    debugRenderAltitudeShiftCorners(false),
    debugExtractRawResources(false)
{}

MapOptions::MapOptions(const std::string &json)
    : MapOptions()
{
    if (!json.empty())
        applyJson(json);
}

void MapOptions::applyJson(const std::string &json)
{
    Json::Value v = stringToJson(json);
    if (v.isMember("controlOptions"))
        controlOptions.applyJson(jsonToString(v["controlOptions"]));
    AJ(maxTexelToPixelScale, asDouble);
    AJ(viewExtentLimitScaleMin, asDouble);
    AJ(viewExtentLimitScaleMax, asDouble);
    AJ(viewExtentThresholdScaleLow, asDouble);
    AJ(viewExtentThresholdScaleHigh, asDouble);
    AJ(tiltLimitAngleLow, asDouble);
    AJ(tiltLimitAngleHigh, asDouble);
    AJ(cameraAltitudeFadeOutFactor, asDouble);
    AJ(navigationLatitudeThreshold, asDouble);
    AJ(navigationPihaViewExtentMult, asDouble);
    AJ(navigationPihaPositionChange, asDouble);
    AJ(renderTilesScale, asDouble);
    AJ(fixedTraversalDistance, asDouble);
    AJ(targetResourcesMemoryKB, asUInt);
    AJ(maxConcurrentDownloads, asUInt);
    AJ(maxResourceProcessesPerTick, asUInt);
    AJ(navigationSamplesPerViewExtent, asUInt);
    AJ(maxFetchRedirections, asUInt);
    AJ(maxFetchRetries, asUInt);
    AJ(fetchFirstRetryTimeOffset, asUInt);
    AJ(fixedTraversalLod, asUInt);
    AJE(navigationType, NavigationType);
    AJE(navigationMode, NavigationMode);
    AJE(traverseModeSurfaces, TraverseMode);
    AJE(traverseModeGeodata, TraverseMode);
    AJ(enableSearchResultsFilter, asBool);
    AJ(enableArbitrarySriRequests, asBool);
    AJ(enableCameraNormalization, asBool);
    AJ(enableCameraAltitudeChanges, asBool);
    AJ(debugDetachedCamera, asBool);
    AJ(debugEnableVirtualSurfaces, asBool);
    AJ(debugEnableSri, asBool);
    AJ(debugSaveCorruptedFiles, asBool);
    AJ(debugFlatShading, asBool);
    AJ(debugValidateGeodataStyles, asBool);
    AJ(debugRenderSurrogates, asBool);
    AJ(debugRenderMeshBoxes, asBool);
    AJ(debugRenderTileBoxes, asBool);
    AJ(debugRenderObjectPosition, asBool);
    AJ(debugRenderTargetPosition, asBool);
    AJ(debugRenderAltitudeShiftCorners, asBool);
    AJ(debugExtractRawResources, asBool);
}

std::string MapOptions::toJson() const
{
    Json::Value v;
    v["controlOptions"] = stringToJson(controlOptions.toJson());
    TJ(maxTexelToPixelScale, asDouble);
    TJ(viewExtentLimitScaleMin, asDouble);
    TJ(viewExtentLimitScaleMax, asDouble);
    TJ(viewExtentThresholdScaleLow, asDouble);
    TJ(viewExtentThresholdScaleHigh, asDouble);
    TJ(tiltLimitAngleLow, asDouble);
    TJ(tiltLimitAngleHigh, asDouble);
    TJ(cameraAltitudeFadeOutFactor, asDouble);
    TJ(navigationLatitudeThreshold, asDouble);
    TJ(navigationPihaViewExtentMult, asDouble);
    TJ(navigationPihaPositionChange, asDouble);
    TJ(renderTilesScale, asDouble);
    TJ(fixedTraversalDistance, asDouble);
    TJ(targetResourcesMemoryKB, asUInt);
    TJ(maxConcurrentDownloads, asUInt);
    TJ(maxResourceProcessesPerTick, asUInt);
    TJ(navigationSamplesPerViewExtent, asUInt);
    TJ(maxFetchRedirections, asUInt);
    TJ(maxFetchRetries, asUInt);
    TJ(fetchFirstRetryTimeOffset, asUInt);
    TJ(fixedTraversalLod, asUInt);
    TJE(navigationType, NavigationType);
    TJE(navigationMode, NavigationMode);
    TJE(traverseModeSurfaces, TraverseMode);
    TJE(traverseModeGeodata, TraverseMode);
    TJ(enableSearchResultsFilter, asBool);
    TJ(enableArbitrarySriRequests, asBool);
    TJ(enableCameraNormalization, asBool);
    TJ(enableCameraAltitudeChanges, asBool);
    TJ(debugDetachedCamera, asBool);
    TJ(debugEnableVirtualSurfaces, asBool);
    TJ(debugEnableSri, asBool);
    TJ(debugSaveCorruptedFiles, asBool);
    TJ(debugFlatShading, asBool);
    TJ(debugValidateGeodataStyles, asBool);
    TJ(debugRenderSurrogates, asBool);
    TJ(debugRenderMeshBoxes, asBool);
    TJ(debugRenderTileBoxes, asBool);
    TJ(debugRenderObjectPosition, asBool);
    TJ(debugRenderTargetPosition, asBool);
    TJ(debugRenderAltitudeShiftCorners, asBool);
    TJ(debugExtractRawResources, asBool);
    return jsonToString(v);
}

} // namespace vts
