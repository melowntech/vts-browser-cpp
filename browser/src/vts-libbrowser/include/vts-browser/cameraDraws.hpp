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

#ifndef CAMERA_DRAWS_HPP_qwedfzugvsdfh
#define CAMERA_DRAWS_HPP_qwedfzugvsdfh

#include <vector>
#include <memory>

#include "camera_common.h"

namespace vts
{

class RenderTask;
class CameraImpl;

class VTS_API DrawTask : public vtsCDrawBase
{
public:
    std::shared_ptr<void> mesh;
    std::shared_ptr<void> texColor;
    std::shared_ptr<void> texMask;

    DrawTask();
    DrawTask(const CameraImpl *m, const RenderTask &r);
    DrawTask(const CameraImpl *m, const RenderTask &r,
        const float *uvClip, float opacity);
};

class VTS_API CameraDraws
{
public:
    // tasks that may be rendered in optimized way without any transparency
    // (may be rendered in any order)
    std::vector<DrawTask> opaque;
    // tasks that need blending enabled for correct rendering
    // (must be rendered in given order)
    std::vector<DrawTask> transparent;
    // geodata
    std::vector<DrawTask> geodata;
    // visualization of debug data
    std::vector<DrawTask> infographics;
    // meshes suitable for collision detection
    // each node's mesh is reported only once
    std::vector<DrawTask> colliders;

    struct VTS_API Camera : public vtsCCameraBase
    {
        Camera();
    } camera;

    CameraDraws();
    void clear();
};

} // namespace vts

#endif
