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

#include "camera.hpp"

namespace vts
{

DrawSurfaceTask::DrawSurfaceTask()
{
    memset((vtsCDrawSurfaceBase*)this, 0, sizeof(vtsCDrawSurfaceBase));
    color[3] = 1;
    vecToRaw(vec4f(-1, -1, 2, 2), uvClip);
}

DrawGeodataTask::DrawGeodataTask()
{
    memset((vtsCDrawGeodataBase*)this, 0, sizeof(vtsCDrawGeodataBase));
}

DrawSimpleTask::DrawSimpleTask()
{
    memset((vtsCDrawSimpleBase*)this, 0, sizeof(vtsCDrawSimpleBase));
    color[3] = 1;
}

CameraDraws::Camera::Camera()
{
    memset(this, 0, sizeof(*this));
}

CameraDraws::CameraDraws()
{}

void CameraDraws::clear()
{
    camera = Camera();
    opaque.clear();
    transparent.clear();
    geodata.clear();
    infographics.clear();
    colliders.clear();
}

RenderSurfaceTask::RenderSurfaceTask() : model(identityMatrix4()),
    uvm(identityMatrix3().cast<float>()),
    color(1, 1, 1, 1), externalUv(false), flatShading(false)
{}

bool RenderSurfaceTask::ready() const
{
    if (mesh && !*mesh)
        return false;
    if (textureColor && !*textureColor)
        return false;
    if (textureMask && !*textureMask)
        return false;
    return true;
}

RenderGeodataTask::RenderGeodataTask()
{}

bool RenderGeodataTask::ready() const
{
    return true;
}

RenderSimpleTask::RenderSimpleTask() : model(identityMatrix4()),
    uvm(identityMatrix3().cast<float>()),
    color(1, 1, 1, 1)
{}

bool RenderSimpleTask::ready() const
{
    if (mesh && !*mesh)
        return false;
    if (textureColor && !*textureColor)
        return false;
    return true;
}

namespace
{

template<class D, class R>
D convert(CameraImpl *impl, const R &task)
{
    assert(task.ready());
    D result;
    if (task.mesh)
        result.mesh = task.mesh->info.userData;
    if (task.textureColor)
        result.texColor = task.textureColor->info.userData;
    mat4f mv = (impl->viewActual * task.model).cast<float>();
    matToRaw(mv, result.mv);
    matToRaw(task.uvm, result.uvm);
    vecToRaw(task.color, result.color);
    return result;
}

} // namespace

DrawSurfaceTask CameraImpl::convert(const RenderSurfaceTask &task)
{
    DrawSurfaceTask result = vts::convert<DrawSurfaceTask,
        RenderSurfaceTask>(this, task);
    if (task.textureMask)
        result.texMask = task.textureMask->info.userData;
    vecToRaw(vec4f(0, 0, 1, 1), result.uvClip);
    vec3f c = vec4to3(task.model * vec4(0, 0, 0, 1)).cast<float>();
    vecToRaw(c, result.center);
    result.flatShading = task.flatShading || options.debugFlatShading;
    result.externalUv = task.externalUv;
    return result;
}

DrawSurfaceTask CameraImpl::convert(const RenderSurfaceTask &task,
    const vec4f &uvClip, float opacity)
{
    DrawSurfaceTask result = convert(task);
    vecToRaw(uvClip, result.uvClip);
    if (opacity == opacity)
        result.color[3] *= opacity;
    return result;
}

DrawGeodataTask CameraImpl::convert(const RenderGeodataTask &task)
{
    DrawGeodataTask result;
    result.geodata = task.geodata->info.userData;
    return result;
}

DrawSimpleTask CameraImpl::convert(const RenderSimpleTask &task)
{
    return vts::convert<DrawSimpleTask, RenderSimpleTask>(this, task);
}

} // namespace vts

