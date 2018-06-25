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

#ifndef DRAWS_HPP_qwedfzugvsdfh
#define DRAWS_HPP_qwedfzugvsdfh

#include <vector>
#include <memory>

#include "draws_common.h"

namespace vts
{

class RenderTask;
class MapImpl;

class VTS_API DrawTask : public vtsCDrawBase
{
public:
    std::shared_ptr<void> mesh;
    std::shared_ptr<void> texColor;
    std::shared_ptr<void> texMask;

    DrawTask();
    DrawTask(const RenderTask &r, const MapImpl *m);
    DrawTask(const RenderTask &r, const float *uvClip, const MapImpl *m);
};

class VTS_API MapDraws
{
public:
    std::vector<DrawTask> opaque;
    std::vector<DrawTask> transparent;
    std::vector<DrawTask> geodata;
    std::vector<DrawTask> infographics;

    std::shared_ptr<void> atmosphereDensityTexture;

    struct VTS_API Camera : public vtsCCameraBase
    {} camera;

    MapDraws();
    void clear();
    void sortOpaqueFrontToBack();
};

} // namespace vts

#endif
