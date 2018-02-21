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

#ifndef DRAWS_H_qwedfzugvsdfh
#define DRAWS_H_qwedfzugvsdfh

#include <vector>
#include <memory>

#include "foundation.hpp"

namespace vts
{

class RenderTask;
class MapImpl;

class VTS_API DrawTask
{
public:
    std::shared_ptr<void> mesh;
    std::shared_ptr<void> texColor;
    std::shared_ptr<void> texMask;
    float mv[16];
    float uvm[9];
    float color[4];
    float uvClip[4];
    bool externalUv;
    bool flatShading;

    DrawTask();
    DrawTask(const RenderTask &r, const MapImpl *m);
    DrawTask(const RenderTask &r, const float *uvClip, const MapImpl *m);
};

class VTS_API MapDraws
{
public:
    std::vector<DrawTask> opaque;
    std::vector<DrawTask> transparent;
    std::vector<DrawTask> Infographic;

    struct Camera
    {
        double view[16];
        double proj[16];
        double eye[3];
        double near, far;
        double aspect;
        double fov; // vertical, degrees
        bool mapProjected;
    } camera;

    MapDraws();
    void clear();
};

} // namespace vts

#endif
