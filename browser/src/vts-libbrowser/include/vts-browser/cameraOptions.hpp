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

#ifndef CAMERA_OPTIONS_HPP_kwegfdzvgsdfj
#define CAMERA_OPTIONS_HPP_kwegfdzvgsdfj

#include <string>

#include "foundation.hpp"

namespace vts
{

class VTS_API CameraOptions
{
public:
    CameraOptions();
    explicit CameraOptions(const std::string &json);
    void applyJson(const std::string &json);
    std::string toJson() const;

    // maximum ratio of texture details to the viewport resolution
    // increasing this ratio yields less detailed map
    //   but reduces memory usage and network bandwidth
    double targetPixelRatioSurfaces = 1.2;
    double targetPixelRatioGeodata = 1.2;

    // nodes this far from the camera frustum will still be included for render
    // this is useful to include shadow casters outside camera frustum
    double cullingOffsetDistance = 0;

    double minSuggestedNearClipPlaneDistance = 10;
    double maxSuggestedNearClipPlaneDistance = 1e100;

    // number of virtual samples to fit the view-extent
    // it is used to determine lod index at which to retrieve
    //   the altitude used to correct camera position
    double samplesForAltitudeLodSelection = 8;

    // maximum distance of meshes emitted for fixed traversal mode
    // defined in physical length units (meters)
    double fixedTraversalDistance = 10000;

    // desired lod used with fixed traversal mode
    uint32 fixedTraversalLod = 15;

    TraverseMode traverseModeSurfaces = TraverseMode::Stable;
    TraverseMode traverseModeGeodata = TraverseMode::Stable;

    bool debugDetachedCamera = false;
    bool debugRenderSurrogates = false;
    bool debugRenderMeshBoxes = false;
    bool debugRenderTileBoxes = false;

    bool debugRenderTileDiagnostics = false;
    bool debugRenderTileGeodataOnly = false;
    bool debugRenderTileBigText = false;
    bool debugRenderTileLod = false;
    bool debugRenderTileIndices = false;
    bool debugRenderTileTexelSize = false;
    bool debugRenderTileTextureSize = false;
    bool debugRenderTileFaces = false;
    bool debugRenderTileSurface = false;
    bool debugRenderTileBoundLayer = false;
    bool debugRenderTileCredits = false;
};

} // namespace vts

#endif
