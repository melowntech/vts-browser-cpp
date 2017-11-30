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

#ifndef FOUNDATION_H_kjebnj
#define FOUNDATION_H_kjebnj

#include <cstdint>

namespace vts
{

typedef std::uint8_t uint8;
typedef std::int8_t sint8;
typedef std::uint16_t uint16;
typedef std::int16_t sint16;
typedef std::uint32_t uint32;
typedef std::int32_t sint32;
typedef std::uint64_t uint64;
typedef std::int64_t sint64;

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
    //   it will zoom out first and zomm back in at the end
    FlyOver,
};

enum class NavigationMode
{
    // constricts the viewer only to limited range of latitudes
    // the camera is always aligned north-up
    // generally, this mode is easier to use
    Azimuthal,

    // the viewer is free to navigato to anywhere, including the poles
    // camera yaw rotation is also unlimited
    Free,

    // starts in the azimuthal mode and switches to the free mode
    //   when the viewer gets too close to any pole,
    //   or when the viewer changes camera orientation
    // it can be reset back to azimuthal with Map::resetNavigationMode()
    Dynamic,

    // actual navigation mode changes with zoom level and has smooth transition
    Seamless,
};

enum class TraverseMode
{
    // this mode downloads each and every lod from top to the required level,
    //   which ensures, that it has at least something to show at all times
    Hierarchical,

    // this mode downloads only the smallest set of data
    Flat,

    // this mode is a compromise
    Balanced,
};

#ifdef _MSVC
#define VTS_API_EXPORT _declspec(dllexport)
#define VTS_API_IMPORT _declspec(dllimport)
#define VTS_THREAD_LOCAL_STORAGE __declspec(thread)
#else
#define VTS_API_EXPORT __attribute__((visibility ("default")))
#define VTS_API_IMPORT __attribute__((visibility ("default")))
#define VTS_THREAD_LOCAL_STORAGE __thread
#endif

#ifdef VTS_BROWSER_BUILD_STATIC
#define VTS_API
#elif VTS_BROWSER_BUILD_SHARED
#define VTS_API VTS_API_EXPORT
#else
#define VTS_API VTS_API_IMPORT
#endif

} // namespace vts

#endif

