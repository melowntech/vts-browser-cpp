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

#include "utilities/json.hpp"
#include "include/vts-browser/mapOptions.hpp"
#include "include/vts-browser/cameraOptions.hpp"
#include "include/vts-browser/navigationOptions.hpp"

namespace vts
{

MapCreateOptions::MapCreateOptions() :
    clientId("undefined-vts-browser-cpp"),
    searchUrlFallback("https://eu-n1.windyty.com/search.php?format=json"
                       "&addressdetails=1&limit=20&q={value}"),
    searchSrsFallback("+proj=longlat +datum=WGS84 +nodefs"),
    diskCache(true),
    hashCachePaths(true),
    searchUrlFallbackOutsideEarth(false),
    browserOptionsSearchUrls(true),
    atmosphereDensityTexture(true)
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
    AJ(diskCache, asBool);
    AJ(hashCachePaths, asBool);
    AJ(searchUrlFallbackOutsideEarth, asBool);
    AJ(browserOptionsSearchUrls, asBool);
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
    TJ(diskCache, asBool);
    TJ(hashCachePaths, asBool);
    TJ(searchUrlFallbackOutsideEarth, asBool);
    TJ(browserOptionsSearchUrls, asBool);
    return jsonToString(v);
}

MapRuntimeOptions::MapRuntimeOptions() :
    renderTilesScale(1.001),
    targetResourcesMemoryKB(0),
    maxConcurrentDownloads(25),
    maxResourceProcessesPerTick(10),
    maxFetchRedirections(5),
    maxFetchRetries(5),
    fetchFirstRetryTimeOffset(1),
    searchResultsFiltering(true),
    debugVirtualSurfaces(true),
    debugSaveCorruptedFiles(false),
    debugValidateGeodataStyles(true),
    debugCoarsenessDisks(true),
    debugExtractRawResources(false)
{}

MapRuntimeOptions::MapRuntimeOptions(const std::string &json)
    : MapRuntimeOptions()
{
    if (!json.empty())
        applyJson(json);
}

void MapRuntimeOptions::applyJson(const std::string &json)
{
    Json::Value v = stringToJson(json);
    AJ(renderTilesScale, asDouble);
    AJ(targetResourcesMemoryKB, asUInt);
    AJ(maxConcurrentDownloads, asUInt);
    AJ(maxResourceProcessesPerTick, asUInt);
    AJ(maxFetchRedirections, asUInt);
    AJ(maxFetchRetries, asUInt);
    AJ(fetchFirstRetryTimeOffset, asUInt);
    AJ(searchResultsFiltering, asBool);
    AJ(debugVirtualSurfaces, asBool);
    AJ(debugSaveCorruptedFiles, asBool);
    AJ(debugValidateGeodataStyles, asBool);
    AJ(debugCoarsenessDisks, asBool);
    AJ(debugExtractRawResources, asBool);
}

std::string MapRuntimeOptions::toJson() const
{
    Json::Value v;
    TJ(renderTilesScale, asDouble);
    TJ(targetResourcesMemoryKB, asUInt);
    TJ(maxConcurrentDownloads, asUInt);
    TJ(maxResourceProcessesPerTick, asUInt);
    TJ(maxFetchRedirections, asUInt);
    TJ(maxFetchRetries, asUInt);
    TJ(fetchFirstRetryTimeOffset, asUInt);
    TJ(searchResultsFiltering, asBool);
    TJ(debugVirtualSurfaces, asBool);
    TJ(debugSaveCorruptedFiles, asBool);
    TJ(debugValidateGeodataStyles, asBool);
    TJ(debugCoarsenessDisks, asBool);
    TJ(debugExtractRawResources, asBool);
    return jsonToString(v);
}

CameraOptions::CameraOptions() :
    maxTexelToPixelScale(1.2),
    lodBlendingDuration(5.0),
    fixedTraversalDistance(10000),
    fixedTraversalLod(15),
    balancedGridLodOffset(5),
    balancedGridNeighborsDistance(1),
    traverseModeSurfaces(TraverseMode::Balanced),
    traverseModeGeodata(TraverseMode::Hierarchical),
    lodBlending(true),
    debugDetachedCamera(false),
    debugFlatShading(false),
    debugRenderSurrogates(false),
    debugRenderMeshBoxes(false),
    debugRenderTileBoxes(false),
    debugRenderSubtileBoxes(false)
{}

CameraOptions::CameraOptions(const std::string &json)
    : CameraOptions()
{
    if (!json.empty())
        applyJson(json);
}

void CameraOptions::applyJson(const std::string &json)
{
    Json::Value v = stringToJson(json);
    AJ(maxTexelToPixelScale, asDouble);
    AJ(lodBlendingDuration, asDouble);
    AJ(fixedTraversalDistance, asDouble);
    AJ(fixedTraversalLod, asUInt);
    AJ(balancedGridLodOffset, asUInt);
    AJ(balancedGridNeighborsDistance, asUInt);
    AJE(traverseModeSurfaces, TraverseMode);
    AJE(traverseModeGeodata, TraverseMode);
    AJ(lodBlending, asBool);
    AJ(debugDetachedCamera, asBool);
    AJ(debugFlatShading, asBool);
    AJ(debugRenderSurrogates, asBool);
    AJ(debugRenderMeshBoxes, asBool);
    AJ(debugRenderTileBoxes, asBool);
    AJ(debugRenderSubtileBoxes, asBool);
}

std::string CameraOptions::toJson() const
{
    Json::Value v;
    TJ(maxTexelToPixelScale, asDouble);
    TJ(lodBlendingDuration, asDouble);
    TJ(fixedTraversalDistance, asDouble);
    TJ(fixedTraversalLod, asUInt);
    TJ(balancedGridLodOffset, asUInt);
    TJ(balancedGridNeighborsDistance, asUInt);
    TJE(traverseModeSurfaces, TraverseMode);
    TJE(traverseModeGeodata, TraverseMode);
    TJ(lodBlending, asBool);
    TJ(debugDetachedCamera, asBool);
    TJ(debugFlatShading, asBool);
    TJ(debugRenderSurrogates, asBool);
    TJ(debugRenderMeshBoxes, asBool);
    TJ(debugRenderTileBoxes, asBool);
    TJ(debugRenderSubtileBoxes, asBool);
    return jsonToString(v);
}

NavigationOptions::NavigationOptions() :
    sensitivityPan(1),
    sensitivityZoom(1),
    sensitivityRotate(1),
    inertiaPan(0.9),
    inertiaZoom(0.9),
    inertiaRotate(0.9),
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
    navigationSamplesPerViewExtent(8),
    navigationType(NavigationType::Quick),
    navigationMode(NavigationMode::Seamless),
    cameraNormalization(true),
    cameraAltitudeChanges(true),
    debugRenderObjectPosition(false),
    debugRenderTargetPosition(false),
    debugRenderAltitudeShiftCorners(false)
{}

NavigationOptions::NavigationOptions(const std::string &json)
    : NavigationOptions()
{
    if (!json.empty())
        applyJson(json);
}

void NavigationOptions::applyJson(const std::string &json)
{
    Json::Value v = stringToJson(json);
    AJ(sensitivityPan, asDouble);
    AJ(sensitivityZoom, asDouble);
    AJ(sensitivityRotate, asDouble);
    AJ(inertiaPan, asDouble);
    AJ(inertiaZoom, asDouble);
    AJ(inertiaRotate, asDouble);
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
    AJ(navigationSamplesPerViewExtent, asUInt);
    AJE(navigationType, NavigationType);
    AJE(navigationMode, NavigationMode);
    AJ(cameraNormalization, asBool);
    AJ(cameraAltitudeChanges, asBool);
    AJ(debugRenderObjectPosition, asBool);
    AJ(debugRenderTargetPosition, asBool);
    AJ(debugRenderAltitudeShiftCorners, asBool);
}

std::string NavigationOptions::toJson() const
{
    Json::Value v;
    TJ(sensitivityPan, asDouble);
    TJ(sensitivityZoom, asDouble);
    TJ(sensitivityRotate, asDouble);
    TJ(inertiaPan, asDouble);
    TJ(inertiaZoom, asDouble);
    TJ(inertiaRotate, asDouble);
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
    TJ(navigationSamplesPerViewExtent, asUInt);
    TJE(navigationType, NavigationType);
    TJE(navigationMode, NavigationMode);
    TJ(cameraAltitudeChanges, asBool);
    TJ(cameraNormalization, asBool);
    TJ(debugRenderObjectPosition, asBool);
    TJ(debugRenderTargetPosition, asBool);
    TJ(debugRenderAltitudeShiftCorners, asBool);
    return jsonToString(v);
}

} // namespace vts
