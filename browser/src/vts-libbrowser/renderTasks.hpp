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

#ifndef RENDERTASKS_HPP_sd564fhj8
#define RENDERTASKS_HPP_sd564fhj8

#include "include/vts-browser/math.hpp"

namespace vts
{

class GpuMesh;
class GpuTexture;

class RenderSurfaceTask
{
public:
    std::shared_ptr<GpuMesh> mesh;
    std::shared_ptr<GpuTexture> textureColor;
    std::shared_ptr<GpuTexture> textureMask;
    std::string boundLayerId;
    mat4 model;
    mat3f uvm;
    vec4f color;
    bool externalUv = false;
    bool flatShading = false;

    RenderSurfaceTask();
    bool ready() const;
};

class RenderInfographicsTask
{
public:
    std::shared_ptr<GpuMesh> mesh;
    std::shared_ptr<GpuTexture> textureColor;
    mat4 model;
    vec4f color;

    RenderInfographicsTask();
    bool ready() const;
};

class RenderColliderTask
{
public:
    std::shared_ptr<GpuMesh> mesh;
    mat4 model;

    RenderColliderTask();
    bool ready() const;
};

} // namespace vts

#endif
