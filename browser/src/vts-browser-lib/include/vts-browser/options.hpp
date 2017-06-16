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

VTS_API void setLogMask(const std::string &mask);
VTS_API void setLogConsole(bool enable);
VTS_API void setLogFile(const std::string &filename);

class VTS_API MapCreateOptions
{
public:
    MapCreateOptions(const std::string &clientId);
    
    std::string clientId;
    std::string cachePath;
    bool disableCache;
    bool hashCachePaths;
};

class VTS_API MapOptions
{
public:
    MapOptions();
    
    enum class NavigationMode
    {
        Azimuthal,
        Free,
        Dynamic,
    };
    
    std::string searchUrl;
    
    double maxTexelToPixelScale;
    double positionViewExtentMin;
    double positionViewExtentMax;
    double cameraSensitivityPan;
    double cameraSensitivityZoom;
    double cameraSensitivityRotate;
    double cameraInertiaPan;
    double cameraInertiaZoom;
    double cameraInertiaRotate;
    double navigationLatitudeThreshold;
    uint64 maxResourcesMemory;
    uint32 maxConcurrentDownloads;
    uint32 maxNodeUpdatesPerTick;
    uint32 maxResourceProcessesPerTick;
    uint32 navigationSamplesPerViewExtent;
    uint32 maxFetchRedirections;
    uint32 maxFetchRetries;
    
    NavigationMode navigationMode;
    
    bool renderSurrogates;
    bool renderMeshBoxes;
    bool renderTileBoxes;
    bool renderObjectPosition;
    bool searchResultsFilter;
    bool enableRuntimeResourceExpiration; // experimental
    
    bool debugDetachedCamera;
    bool debugDisableMeta5;
    bool debugSaveCorruptedFiles;
};

class VTS_API MapCallbacks
{
public:
    // function callback to upload a texture to gpu
    std::function<void(class ResourceInfo &, const class GpuTextureSpec &)>
            loadTexture;
    /// function callback to upload a mesh to gpu
    std::function<void(class ResourceInfo &, const class GpuMeshSpec &)>
            loadMesh;
    
    /// function callbacks for camera overrides (all in physical srs)
    std::function<void(double*)> cameraOverrideEye;
    std::function<void(double*)> cameraOverrideTarget;
    std::function<void(double*)> cameraOverrideUp;
    std::function<void(double&, double&, double&, double&)>
                                        cameraOverrideFovAspectNearFar;
    std::function<void(double*)> cameraOverrideView;
    std::function<void(double*)> cameraOverrideProj;
};

} // namespace vts

#endif
