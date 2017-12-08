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

#ifndef OPTIONS_H_kwegfdzvgsdfj
#define OPTIONS_H_kwegfdzvgsdfj

#include <string>
#include <functional>

#include "foundation.hpp"

namespace vts
{

// these options are passed to the map when it is beeing created
//   and are immutable during the lifetime of the map
class VTS_API MapCreateOptions
{
public:
    MapCreateOptions();

    // application id that is sent with all resources
    // to be used by authentication servers
    std::string clientId;

    // path to a directory in which to store all downloaded files
    // leave it empty to use default ($HOME/.cache/vts-browser)
    std::string cachePath;

    // search url (and srs) are usually provided in browser options in mapconfig
    // however, if they are not provided, these fallbacks are used
    std::string searchUrlFallback;
    std::string searchSrsFallback;

    // custom srs definitions that you may use
    //   for additional coordinate transformations
    std::string customSrs1;
    std::string customSrs2;

    // true to disable the hard drive cache entirely
    bool disableCache;

    // true -> use new scheme for naming (hashing) files
    //         in a hierarchy of directories in the cache
    // false -> use old scheme where the name of the downloaded resource
    //          is clearly reflected in the cached file name
    bool hashCachePaths;

    // true to try detect Earth and only use the fallbacks on it
    bool disableSearchUrlFallbackOutsideEarth;

    // true to disable search url (and srs) from mapconfig and use the fallbacks
    bool disableBrowserOptionsSearchUrls;
};

// options of the map which may be changed anytime
// (although, some of the options may take effect slowly
//    as some internal caches are beeing rebuild)
class VTS_API MapOptions
{
public:
    MapOptions();

    // maximum ratio of texture details to the viewport resolution
    // increasing this ratio yealds less detailed map
    //   but reduces memory usage and memory bandwith
    double maxTexelToPixelScale;

    // addition to ratio of kept textures res. in the balanced traverse mode
    // increasing this ratio will store more lods above the optimal one
    double maxTexelToPixelScaleBalancedAddition;

    // lower and upper limit for view-extent
    // expressed as multiplicative factor of planet major radius
    double viewExtentLimitScaleMin;
    double viewExtentLimitScaleMax;

    // view-extent thresholds at which the camera normalization takes effect
    // expressed as multiplicative factor of planet major radius
    double viewExtentThresholdScaleLow;
    double viewExtentThresholdScaleHigh;

    // camera tilt limits (eg. 270 - 360)
    double tiltLimitAngleLow;
    double tiltLimitAngleHigh;

    // multiplier that is applied to mouse input for the respective actions
    double cameraSensitivityPan;
    double cameraSensitivityZoom;
    double cameraSensitivityRotate;

    // innertia coefficients [0 - 1) for smoothing the navigation changes
    // inertia of zero makes all changes to apply immediately
    // while inertia of almost one makes the moves very smooth and slow
    double cameraInertiaPan;
    double cameraInertiaZoom;
    double cameraInertiaRotate;

    // multiplicative factor at which camera altitude will converge to terrain
    //   when panning or zooming
    // range 0 (off) to 1 (fast)
    double cameraAltitudeFadeOutFactor;

    // latitude threshold (0 - 90) used for azimuthal navigation
    double navigationLatitudeThreshold;

    // maximum view-extent multiplier allowed for PIHA, must be larger than 1
    double navigationPihaViewExtentMult;
    // maximum position change allowed for PIHA, must be positive
    double navigationPihaPositionChange;

    // relative scale of every tile.
    // small up-scale may reduce occasional holes on tile borders.
    double renderTilesScale;

    // memory threshold at which resources start to be released
    uint64 targetResourcesMemory; // bytes

    // maximum size of the queue for the resources to be downloaded
    uint32 maxConcurrentDownloads;

    // maximum number of resources processed per dataTick
    uint32 maxResourceProcessesPerTick;

    // number of virtual samples to fit the view-extent
    // it is used to determine lod index at which to retrieve
    //   the altitude used to correct camera position
    uint32 navigationSamplesPerViewExtent;

    // maximum number of redirections before the download fails
    // this is to prevent infinite loops
    uint32 maxFetchRedirections;

    // maximum number of attempts to redownload a resource
    uint32 maxFetchRetries;

    // delay in seconds for first resource download retry
    // each subsequent retry is delayed twice as long as before
    uint32 fetchFirstRetryTimeOffset;

    NavigationType navigationType;
    NavigationMode navigationMode;
    TraverseMode traverseMode;

    // to improve search results relevance, the results are further
    //   filtered and reordered
    // set this to false to prevent such filtering
    bool enableSearchResultsFilter;

    // some resources may intentionally change over time,
    //   thus they need to be reloaded when they expire
    // enable this *experimental* feature to allow resource expiration
    //   even after they were successfully loaded into gpu memory
    bool enableRuntimeResourceExpiration;

    // some applications may be changing viewer locations too rapidly
    //   and it may make the SRI optimizations inefficient
    // setting this option to false will allow the use of SRI
    //   only when the mapconfig changes
    bool enableArbitrarySriRequests;

    // setting this to false will disable that camera tilt and yaw
    //   are limited when zoomed out
    bool enableCameraNormalization;

    bool debugDetachedCamera;
    bool debugDisableVirtualSurfaces;
    bool debugDisableSri;
    bool debugSaveCorruptedFiles;
    bool debugFlatShading;

    bool debugRenderSurrogates;
    bool debugRenderMeshBoxes;
    bool debugRenderTileBoxes;
    bool debugRenderObjectPosition;
    bool debugRenderTargetPosition;
    bool debugRenderAltitudeShiftCorners;
    bool debugRenderNoMeshes;
};

class VTS_API MapCallbacks
{
public:
    // function callback to upload a texture to gpu
    // called from Map::dataTick()
    std::function<void(class ResourceInfo &, class GpuTextureSpec &)>
            loadTexture;

    // function callback to upload a mesh to gpu
    // called from Map::dataTick()
    std::function<void(class ResourceInfo &, class GpuMeshSpec &)>
            loadMesh;

    // function callback when the mapconfig is ready
    // called from Map::renderTickPrepare()
    std::function<void()> mapconfigReady;

    // function callbacks for camera overrides (all in physical srs)
    // these callbacks are called from the Map::renderTickRender()
    std::function<void(double*)> cameraOverrideEye;
    std::function<void(double*)> cameraOverrideTarget;
    std::function<void(double*)> cameraOverrideUp;
    std::function<void(double&, double&, double&, double&)>
                                        cameraOverrideFovAspectNearFar;
    std::function<void(double*)> cameraOverrideView;
    std::function<void(double*)> cameraOverrideProj;
};

VTS_API extern std::function<const char *(const char *)> projFinder;

} // namespace vts

#endif
