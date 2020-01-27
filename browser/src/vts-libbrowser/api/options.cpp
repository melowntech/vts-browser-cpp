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

#include "utility/enum-io.hpp"
#include "../utilities/json.hpp"
#include "../include/vts-browser/mapOptions.hpp"
#include "../include/vts-browser/cameraOptions.hpp"
#include "../include/vts-browser/navigationOptions.hpp"
#include "../include/vts-browser/fetcher.hpp"
#include "../utilities/detectLanguage.hpp"

namespace vts
{

MapCreateOptions::MapCreateOptions() :
    clientId("undefined-vts-browser-cpp"),
    geodataFontFallback(
        "//cdn.melown.com/libs/vtsjs/fonts/noto-basic/1.0.0/noto.fnt"),
    searchUrlFallback("https://eu-n1.windyty.com/search.php?format=json"
                       "&addressdetails=1&limit=20&q={value}"),
    searchSrsFallback("+proj=longlat +datum=WGS84 +nodefs"),
#ifdef VTS_EMBEDDED
    diskCache(false),
#else
    diskCache(true),
#endif // VTS_EMBEDDED
    hashCachePaths(true),
    searchUrlFallbackOutsideEarth(false),
    browserOptionsSearchUrls(true),
    atmosphereDensityTexture(true),
    debugUseExtraThreads(true)
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
    AJ(geodataFontFallback, asString);
    AJ(searchUrlFallback, asString);
    AJ(searchSrsFallback, asString);
    AJ(customSrs1, asString);
    AJ(customSrs2, asString);
    AJ(diskCache, asBool);
    AJ(hashCachePaths, asBool);
    AJ(searchUrlFallbackOutsideEarth, asBool);
    AJ(browserOptionsSearchUrls, asBool);
    AJ(atmosphereDensityTexture, asBool);
    AJ(debugUseExtraThreads, asBool);
}

std::string MapCreateOptions::toJson() const
{
    Json::Value v;
    TJ(clientId, asString);
    TJ(cachePath, asString);
    TJ(geodataFontFallback, asString);
    TJ(searchUrlFallback, asString);
    TJ(searchSrsFallback, asString);
    TJ(customSrs1, asString);
    TJ(customSrs2, asString);
    TJ(diskCache, asBool);
    TJ(hashCachePaths, asBool);
    TJ(searchUrlFallbackOutsideEarth, asBool);
    TJ(browserOptionsSearchUrls, asBool);
    TJ(atmosphereDensityTexture, asBool);
    TJ(debugUseExtraThreads, asBool);
    return jsonToString(v);
}

MapRuntimeOptions::MapRuntimeOptions() :
    language(detectLanguage()),
    pixelsPerInch(96),
    renderTilesScale(1.001),
    targetResourcesMemoryKB(0),
    maxConcurrentDownloads(25),
    maxResourceProcessesPerTick(3),
    maxFetchRedirections(5),
    maxFetchRetries(5),
    fetchFirstRetryTimeOffset(1),
    measurementUnitsSystem(detectMeasurementSystem()),
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
    AJ(language, asString);
    AJ(pixelsPerInch, asDouble);
    AJ(renderTilesScale, asDouble);
    AJ(targetResourcesMemoryKB, asUInt);
    AJ(maxConcurrentDownloads, asUInt);
    AJ(maxResourceProcessesPerTick, asUInt);
    AJ(maxFetchRedirections, asUInt);
    AJ(maxFetchRetries, asUInt);
    AJ(fetchFirstRetryTimeOffset, asUInt);
    AJ(measurementUnitsSystem, asUInt);
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
    TJ(language, asString);
    TJ(pixelsPerInch, asDouble);
    TJ(renderTilesScale, asDouble);
    TJ(targetResourcesMemoryKB, asUInt);
    TJ(maxConcurrentDownloads, asUInt);
    TJ(maxResourceProcessesPerTick, asUInt);
    TJ(maxFetchRedirections, asUInt);
    TJ(maxFetchRetries, asUInt);
    TJ(fetchFirstRetryTimeOffset, asUInt);
    TJ(measurementUnitsSystem, asUInt);
    TJ(searchResultsFiltering, asBool);
    TJ(debugVirtualSurfaces, asBool);
    TJ(debugSaveCorruptedFiles, asBool);
    TJ(debugValidateGeodataStyles, asBool);
    TJ(debugCoarsenessDisks, asBool);
    TJ(debugExtractRawResources, asBool);
    return jsonToString(v);
}

CameraOptions::CameraOptions() :
    targetPixelRatioSurfaces(1.2),
    targetPixelRatioGeodata(1.2),
    cullingOffsetDistance(0.0),
    lodBlendingDuration(1.0),
    samplesForAltitudeLodSelection(8),
    fixedTraversalDistance(10000),
    fixedTraversalLod(15),
    balancedGridLodOffset(5),
    balancedGridNeighborsDistance(1),
    lodBlending(2),
    traverseModeSurfaces(TraverseMode::Balanced),
    traverseModeGeodata(TraverseMode::Stable),
    lodBlendingTransparent(false),
    debugDetachedCamera(false),
    debugFlatShading(false),
    debugRenderSurrogates(false),
    debugRenderMeshBoxes(false),
    debugRenderTileBoxes(false),
    debugRenderSubtileBoxes(false),
    debugRenderTileDiagnostics(false),
    debugRenderTileGeodataOnly(false),
    debugRenderTileBigText(false),
    debugRenderTileLod(false),
    debugRenderTileIndices(false),
    debugRenderTileTexelSize(false),
    debugRenderTileTextureSize(false),
    debugRenderTileFaces(false),
    debugRenderTileSurface(false),
    debugRenderTileBoundLayer(false),
    debugRenderTileCredits(false)
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
    AJ(targetPixelRatioSurfaces, asDouble);
    AJ(targetPixelRatioGeodata, asDouble);
    AJ(cullingOffsetDistance, asDouble);
    AJ(lodBlendingDuration, asDouble);
    AJ(samplesForAltitudeLodSelection, asDouble);
    AJ(fixedTraversalDistance, asDouble);
    AJ(fixedTraversalLod, asUInt);
    AJ(balancedGridLodOffset, asUInt);
    AJ(balancedGridNeighborsDistance, asUInt);
    AJ(lodBlending, asUInt);
    AJE(traverseModeSurfaces, TraverseMode);
    AJE(traverseModeGeodata, TraverseMode);
    AJ(lodBlendingTransparent, asBool);
    AJ(debugDetachedCamera, asBool);
    AJ(debugFlatShading, asBool);
    AJ(debugRenderSurrogates, asBool);
    AJ(debugRenderMeshBoxes, asBool);
    AJ(debugRenderTileBoxes, asBool);
    AJ(debugRenderSubtileBoxes, asBool);
    AJ(debugRenderTileDiagnostics, asBool);
    AJ(debugRenderTileGeodataOnly, asBool);
    AJ(debugRenderTileBigText, asBool);
    AJ(debugRenderTileLod, asBool);
    AJ(debugRenderTileIndices, asBool);
    AJ(debugRenderTileTexelSize, asBool);
    AJ(debugRenderTileTextureSize, asBool);
    AJ(debugRenderTileFaces, asBool);
    AJ(debugRenderTileSurface, asBool);
    AJ(debugRenderTileBoundLayer, asBool);
    AJ(debugRenderTileCredits, asBool);
}

std::string CameraOptions::toJson() const
{
    Json::Value v;
    TJ(targetPixelRatioSurfaces, asDouble);
    TJ(targetPixelRatioGeodata, asDouble);
    TJ(cullingOffsetDistance, asDouble);
    TJ(lodBlendingDuration, asDouble);
    TJ(samplesForAltitudeLodSelection, asDouble);
    TJ(fixedTraversalDistance, asDouble);
    TJ(fixedTraversalLod, asUInt);
    TJ(balancedGridLodOffset, asUInt);
    TJ(balancedGridNeighborsDistance, asUInt);
    TJ(lodBlending, asUInt);
    TJE(traverseModeSurfaces, TraverseMode);
    TJE(traverseModeGeodata, TraverseMode);
    TJ(lodBlendingTransparent, asBool);
    TJ(debugDetachedCamera, asBool);
    TJ(debugFlatShading, asBool);
    TJ(debugRenderSurrogates, asBool);
    TJ(debugRenderMeshBoxes, asBool);
    TJ(debugRenderTileBoxes, asBool);
    TJ(debugRenderSubtileBoxes, asBool);
    TJ(debugRenderTileDiagnostics, asBool);
    TJ(debugRenderTileGeodataOnly, asBool);
    TJ(debugRenderTileBigText, asBool);
    TJ(debugRenderTileLod, asBool);
    TJ(debugRenderTileIndices, asBool);
    TJ(debugRenderTileTexelSize, asBool);
    TJ(debugRenderTileTextureSize, asBool);
    TJ(debugRenderTileFaces, asBool);
    TJ(debugRenderTileSurface, asBool);
    TJ(debugRenderTileBoundLayer, asBool);
    TJ(debugRenderTileCredits, asBool);
    return jsonToString(v);
}

NavigationOptions::NavigationOptions() :
    sensitivityPan(1),
    sensitivityZoom(1),
    sensitivityRotate(1),
    inertiaPan(0.9),
    inertiaZoom(0.9),
    inertiaRotate(0.9),
    viewExtentLimitScaleMin(0.00001175917), // 75 meters on earth
    viewExtentLimitScaleMax(2.35183443086), // 1.5e7 meters on earth
    viewExtentThresholdScaleLow(0.1097522734), // 700 000 meters on earth
    viewExtentThresholdScaleHigh(0.20382565067), // 1 300 000 meters on earth
    tiltLimitAngleLow(-90),
    tiltLimitAngleHigh(-10),
    altitudeFadeOutFactor(0.5),
    azimuthalLatitudeThreshold(80),
    flyOverSpikinessFactor(2.5),
    flyOverMotionChangeFraction(0.5),
    flyOverRotationChangeSpeed(0.5),
    type(NavigationType::Quick),
    mode(NavigationMode::Seamless),
    enableNormalization(true),
    enableAltitudeCorrections(true),
    fpsCompensation(true),
    debugRenderObjectPosition(false),
    debugRenderTargetPosition(false),
    debugRenderAltitudeSurrogates(false),
    debugRenderCameraObstructionSurrogates(false)
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
    AJ(altitudeFadeOutFactor, asDouble);
    AJ(azimuthalLatitudeThreshold, asDouble);
    AJ(flyOverSpikinessFactor, asDouble);
    AJ(flyOverMotionChangeFraction, asDouble);
    AJ(flyOverRotationChangeSpeed, asDouble);
    AJE(type, NavigationType);
    AJE(mode, NavigationMode);
    AJ(enableNormalization, asBool);
    AJ(enableAltitudeCorrections, asBool);
    AJ(fpsCompensation, asBool);
    AJ(debugRenderObjectPosition, asBool);
    AJ(debugRenderTargetPosition, asBool);
    AJ(debugRenderAltitudeSurrogates, asBool);
    AJ(debugRenderCameraObstructionSurrogates, asBool);
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
    TJ(altitudeFadeOutFactor, asDouble);
    TJ(azimuthalLatitudeThreshold, asDouble);
    TJ(flyOverSpikinessFactor, asDouble);
    TJ(flyOverMotionChangeFraction, asDouble);
    TJ(flyOverRotationChangeSpeed, asDouble);
    TJE(type, NavigationType);
    TJE(mode, NavigationMode);
    TJ(enableAltitudeCorrections, asBool);
    TJ(enableNormalization, asBool);
    TJ(fpsCompensation, asBool);
    TJ(debugRenderObjectPosition, asBool);
    TJ(debugRenderTargetPosition, asBool);
    TJ(debugRenderAltitudeSurrogates, asBool);
    TJ(debugRenderCameraObstructionSurrogates, asBool);
    return jsonToString(v);
}

FetcherOptions::FetcherOptions() :
    threads(1),
    timeout(30000),
    extraFileLog(false),
    maxHostConnections(0),
    maxTotalConnections(0),
    maxCacheConections(0),
    pipelining(2)
{}

FetcherOptions::FetcherOptions(const std::string &json)
    : FetcherOptions()
{
    if (!json.empty())
        applyJson(json);
}

void FetcherOptions::applyJson(const std::string &json)
{
    Json::Value v = stringToJson(json);
    AJ(threads, asUInt);
    AJ(timeout, asUInt);
    AJ(extraFileLog, asBool);
    AJ(maxHostConnections, asUInt);
    AJ(maxTotalConnections, asUInt);
    AJ(maxCacheConections, asUInt);
    AJ(pipelining, asUInt);
}

std::string FetcherOptions::toJson() const
{
    Json::Value v;
    TJ(threads, asUInt);
    TJ(timeout, asUInt);
    TJ(extraFileLog, asBool);
    TJ(maxHostConnections, asUInt);
    TJ(maxTotalConnections, asUInt);
    TJ(maxCacheConections, asUInt);
    TJ(pipelining, asUInt);
    return jsonToString(v);
}

} // namespace vts
