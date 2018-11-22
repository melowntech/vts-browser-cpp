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

#ifndef MAP_CALLBACKS_HPP_skjgfjshfk
#define MAP_CALLBACKS_HPP_skjgfjshfk

#include <functional>

#include "foundation.hpp"

namespace vts
{

class VTS_API MapCallbacks
{
public:
    // function callback to upload a texture to gpu
    // invoked from Map::dataTick()
    std::function<void(class ResourceInfo &, class GpuTextureSpec &)>
            loadTexture;

    // function callback to upload a mesh to gpu
    // invoked from Map::dataTick()
    std::function<void(class ResourceInfo &, class GpuMeshSpec &)>
            loadMesh;

    // function callback when the mapconfig is downloaded
    // invoked from Map::renderTick()
    // suitable to change view, position, etc.
    std::function<void()> mapconfigAvailable;

    // function callback when the mapconfig and all other required
    //   external definitions are initialized
    // invoked from Map::renderTick()
    // suitable to start navigation etc.
    std::function<void()> mapconfigReady;
};

VTS_API extern std::function<const char *(const char *)> projFinder;

} // namespace vts

#endif
