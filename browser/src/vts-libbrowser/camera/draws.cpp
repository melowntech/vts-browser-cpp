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

#include "../camera.hpp"
#include "../renderTasks.hpp"
#include "../gpuResource.hpp"

namespace vts
{

DrawSurfaceTask::DrawSurfaceTask()
{
    memset((vtsCDrawSurfaceBase*)this, 0,
        sizeof(vtsCDrawSurfaceBase));
    color[3] = 1;
    blendingCoverage = 1;
    vecToRaw(vec4f(-1, -1, 2, 2), uvClip);
}

DrawGeodataTask::DrawGeodataTask()
{}

DrawInfographicsTask::DrawInfographicsTask()
{
    memset((vtsCDrawInfographicsBase*)this, 0,
        sizeof(vtsCDrawInfographicsBase));
    color[3] = 1;
}

DrawColliderTask::DrawColliderTask()
{
    memset((vtsCDrawColliderBase*)this, 0,
        sizeof(vtsCDrawColliderBase));
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
    color(1, 1, 1, 1)
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

RenderInfographicsTask::RenderInfographicsTask() : model(identityMatrix4()),
    color(1, 1, 1, 1)
{}

bool RenderInfographicsTask::ready() const
{
    if (mesh && !*mesh)
        return false;
    if (textureColor && !*textureColor)
        return false;
    return true;
}

RenderColliderTask::RenderColliderTask() : model(identityMatrix4())
{}

bool RenderColliderTask::ready() const
{
    if (mesh && !*mesh)
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
        result.mesh = task.mesh->getUserData();
    if (task.textureColor)
        result.texColor = task.textureColor->getUserData();
    mat4f mv = mat4(impl->viewActual * task.model).cast<float>();
    matToRaw(mv, result.mv);
    vecToRaw(task.color, result.color);
    return result;
}

} // namespace

DrawSurfaceTask CameraImpl::convert(const RenderSurfaceTask &task)
{
    DrawSurfaceTask result = vts::convert<DrawSurfaceTask,
        RenderSurfaceTask>(this, task);
    if (task.textureMask)
        result.texMask = task.textureMask->getUserData();
    matToRaw(task.uvm, result.uvm);
    vecToRaw(vec4f(0, 0, 1, 1), result.uvClip);
    vec3f c = vec4to3(vec4(task.model * vec4(0, 0, 0, 1))).cast<float>();
    vecToRaw(c, result.center);
    result.flatShading = task.flatShading || options.debugFlatShading;
    result.externalUv = task.externalUv;
    return result;
}

DrawSurfaceTask CameraImpl::convert(const RenderSurfaceTask &task,
    const vec4f &uvClip, float blendingCoverage)
{
    DrawSurfaceTask result = convert(task);
    vecToRaw(uvClip, result.uvClip);
    result.blendingCoverage = blendingCoverage; // may be nan
    return result;
}

DrawInfographicsTask CameraImpl::convert(const RenderInfographicsTask &task)
{
    return vts::convert<DrawInfographicsTask,
        RenderInfographicsTask>(this, task);
}

DrawColliderTask CameraImpl::convert(const RenderColliderTask &task)
{
    assert(task.ready());
    DrawColliderTask result;
    if (task.mesh)
        result.mesh = task.mesh->getUserData();
    mat4f mv = mat4(viewActual * task.model).cast<float>();
    matToRaw(mv, result.mv);
    return result;
}

} // namespace vts

