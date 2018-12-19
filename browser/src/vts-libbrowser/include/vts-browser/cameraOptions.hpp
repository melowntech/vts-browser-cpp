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
    CameraOptions(const std::string &json);
    void applyJson(const std::string &json);
    std::string toJson() const;

    // maximum ratio of texture details to the viewport resolution
    // increasing this ratio yealds less detailed map
    //   but reduces memory usage and network bandwith
    double maxTexelToPixelScale;

    // nodes this far from the camera frustum will still be included for render
    // this is useful to include shadow casters outside camera frustum
    double cullingOffsetDistance;

    // maximum distance of meshes emited for fixed traversal mode
    // defined in physical length units (metres)
    double fixedTraversalDistance;

    // desired lod used with fixed traversal mode
    uint32 fixedTraversalLod;

    // coarser lod offset for grids for use with balanced traversal
    // -1 to disable grids entirely
    uint32 balancedGridLodOffset;

    // distance to neighbors for grids for use with balanced traversal
    // 0: no neighbors
    // 1: one ring of neighbors (8 total)
    // 2: two rings (24 total)
    // etc.
    uint32 balancedGridNeighborsDistance;

    // enable blending lods to prevend lod popping
    // 0: disable
    // 1: enable, simple
    // 2: enable, detect draws near edges
    uint32 lodBlending;

    // duration of lod blending in frames
    uint32 lodBlendingDuration;

    TraverseMode traverseModeSurfaces;
    TraverseMode traverseModeGeodata;

    bool debugDetachedCamera;
    bool debugFlatShading;
    bool debugRenderSurrogates;
    bool debugRenderMeshBoxes;
    bool debugRenderTileBoxes;
    bool debugRenderSubtileBoxes;
};

} // namespace vts

#endif
