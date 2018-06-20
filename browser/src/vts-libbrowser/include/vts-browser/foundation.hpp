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

#ifndef FOUNDATION_HPP_kjebnj
#define FOUNDATION_HPP_kjebnj

#include "foundation_common.h"

namespace vts
{

enum class Srs
{
    // mesh vertex coordinates are in physical srs
    // eg. geocentric srs
    Physical,

    // map navigation (eg. panning or rotation) are performed in navigation srs
    // eg. geographic where altitude of zero is at ellipsoid
    Navigation,

    // coordinate system for presentation to people
    // eg. geographic with altitude above sea level
    Public,

    // coordinate system used for search
    // generally, you do not need this because search coordinates
    //   are automatically converted to/from navigation srs
    Search,

    // Custom srs for application use
    Custom1,
    Custom2,
};

enum class NavigationType
{
    // navigation changes are applied fully in first Map::renderTickPrepare()
    Instant,

    // navigation changes progressively over time
    // the change applied is large at first and smoothly drops
    Quick,

    // special navigation mode where the camera speed is limited
    // to speed up transitions over large distances,
    //   it will zoom out first and zoom back in at the end
    FlyOver,
};

enum class NavigationMode
{
    // constricts the viewer only to limited range of latitudes
    // the camera is always aligned north-up
    // generally, this mode is easier to use
    Azimuthal,

    // the viewer is free to navigate to anywhere, including the poles
    // camera yaw rotation is also unlimited
    Free,

    // starts in the azimuthal mode and switches to the free mode
    //   when the viewer gets too close to any pole
    //   or when the viewer changes camera orientation
    // it can be reset back to azimuthal with Map::resetNavigationMode()
    Dynamic,

    // actual navigation mode changes with zoom level and has smooth transition
    Seamless,
};

enum class TraverseMode
{
    // disables traversal of the specific feature
    None,

    // Flat mode requires least amount of memory and downloads
    Flat,

    // Hierarchical mode downloads every lod from top to the required level,
    //   this ensures that it has at least something to show at all times
    Hierarchical,

    // Balanced does litle downloads but makes rendering more consistent
    Balanced,

    // Fixed mode completely changes how the traversal works
    //   it will use fixed selected lod (and some coarser where unavailable)
    //   and it will render everything up to some specified distance
    // this mode is designed for use with colliders query
    Fixed,
};

enum class FreeLayerType
{
    Unknown,
    TiledMeshes,
    TiledGeodata,
    MonolithicGeodata,
};

} // namespace vts

#endif

