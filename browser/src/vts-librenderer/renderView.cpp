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

#include <vts-browser/resources.hpp>
#include <vts-browser/cameraDraws.hpp>
#include <vts-browser/celestial.hpp>
#include <vts-browser/camera.hpp>

#include <optick.h>

#include "renderer.hpp"

#define FILTERS
#define SSAO

namespace vts { namespace renderer
{

void clearGlState()
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glPolygonOffset(0, 0);
    CHECK_GL("cleared gl state");
}

void enableClipDistance(bool enable)
{
#ifndef VTSR_NO_CLIP
    if (enable)
    {
        for (int i = 0; i < 4; i++)
            glEnable(GL_CLIP_DISTANCE0 + i);
    }
    else
    {
        for (int i = 0; i < 4; i++)
            glDisable(GL_CLIP_DISTANCE0 + i);
    }
#endif
}

UboCache::UboCache() : current(0), last(0), prev(0)
{
    // initialize the buffer
    data.reserve(1000);
    data.resize(10);
}

UniformBuffer *UboCache::get()
{
    if ((current + 1) % data.size() == prev)
    {
        // grow the buffer
        data.insert(data.begin() + prev, nullptr);
        prev++;
        if (last > current)
            last++;
    }
    auto &c = data[current];
    current = (current + 1) % data.size();
    if (!c)
        c = std::make_unique<UniformBuffer>();
    return &*c;
}

void UboCache::frame()
{
    prev = last;
    last = current;
}

RenderViewImpl::RenderViewImpl(
    Camera *camera, RenderView *api,
    RenderContextImpl *context) :
    camera(camera),
    api(api),
    context(context),
    draws(nullptr),
    body(nullptr),
    atmosphereDensityTexture(nullptr),
    lastUboViewPointer(nullptr),
    elapsedTime(0),
    width(0),
    height(0),
    antialiasingPrev(0),
    frameIndex(0),
    frameRender2BufferId(0),
    colorRender2TexId(0),
    normalRenderTexId(0),
    projected(false),
    lodBlendingWithDithering(false)
{
    depthBuffer.meshQuad = context->meshQuad;
    depthBuffer.shaderCopyDepth = context->shaderCopyDepth;
}

UniformBuffer *RenderViewImpl::useDisposableUbo(uint32 bindIndex,
    void *data, uint32 size)
{
    UniformBuffer *ubo = size > 256
        ? uboCacheLarge.get() : uboCacheSmall.get();
    ubo->bind();
    ubo->load(data, size, GL_DYNAMIC_DRAW);
    ubo->bindToIndex(bindIndex);
    return ubo;
}

void RenderViewImpl::drawSurface(const DrawSurfaceTask &t, bool wireframeSlow)
{
    Texture *tex = (Texture*)t.texColor.get();
    Mesh *m = (Mesh*)t.mesh.get();
    if (!m || !tex)
        return;

    struct mat3x4f
    {
        vec4f data[3];
        mat3x4f(){}
        mat3x4f(const mat3f &m)
        {
            for (int y = 0; y < 3; y++)
                for (int x = 0; x < 3; x++)
                    data[y][x] = m(x, y);
        }
    };

    struct UboSurface
    {
        mat4f p;
        mat4f mv;
        mat3x4f uvMat; // + blendingCoverage
        vec4f uvClip;
        vec4f color;
        vec4si32 flags; // mask, monochromatic, flat shading, uv source, lodBlendingWithDithering, ... frameIndex
    } data;

    data.p = proj.cast<float>();
    data.mv = rawToMat4(t.mv);
    data.uvMat = mat3x4f(rawToMat3(t.uvm));
    data.uvClip = rawToVec4(t.uvClip);
    data.color = rawToVec4(t.color);
    sint32 flags = 0;
    if (t.texMask)
        flags |= 1 << 0;
    if (tex->getGrayscale())
        flags |= 1 << 1;
    if (t.flatShading)
        flags |= 1 << 2;
    if (t.externalUv)
        flags |= 1 << 3;
    if (!std::isnan(t.blendingCoverage))
    {
        if (lodBlendingWithDithering)
        {
            data.uvMat.data[0][3] = t.blendingCoverage;
            flags |= 1 << 4;
        }
        else
            data.color[3] *= t.blendingCoverage;
    }
    data.flags = vec4si32(flags, 0, 0, frameIndex);

    useDisposableUbo(1, data)->setDebugId("UboSurface");

    if (t.texMask)
    {
        glActiveTexture(GL_TEXTURE0 + 1);
        ((Texture*)t.texMask.get())->bind();
        glActiveTexture(GL_TEXTURE0 + 0);
    }
    tex->bind();

    m->bind();
    if (wireframeSlow)
        m->dispatchWireframeSlow();
    else
        m->dispatch();
}

void RenderViewImpl::drawInfographics(const DrawInfographicsTask &t)
{
    Mesh *m = (Mesh*)t.mesh.get();
    if (!m)
        return;

    struct UboInfographics
    {
        mat4f mvp;
        vec4f color;
        vec4f flags; // type, useTexture, useDepth
        vec4f data;
        vec4f data2;
    } data;

    data.mvp = proj.cast<float>() * rawToMat4(t.mv);
    data.color = rawToVec4(t.color);
    data.flags = vec4f(
        t.type,
        !!t.texColor,
        t.type ? 0 : 1,
        0
    );
    data.data = rawToVec4(t.data);
    data.data2 = rawToVec4(t.data2);

    useDisposableUbo(1, data)->setDebugId("UboInfographics");

    if (t.texColor)
    {
        Texture *tex = (Texture*)t.texColor.get();
        tex->bind();
    }

    m->bind();
    m->dispatch();
}

void RenderViewImpl::updateFramebuffers()
{
    OPTICK_EVENT();

    #ifdef FILTERS
        bool filtersUsed = (options.filterAA || options.filterDOF || options.filterSSAO || options.filterFX);

        uint32 antialiasingCur = filtersUsed ? 1 : options.antialiasingSamples;

        if (options.filterAA > 0)
            antialiasingCur = 1;

        if (options.width != width || options.height != height
            || antialiasingCur != antialiasingPrev || (filtersUsed && !colorRender2TexId) )
        {

            width = options.width;
            height = options.height;
            antialiasingPrev = std::max(std::min(antialiasingCur,
            maxAntialiasingSamples), 1u);

    #else 
        if (options.width != width || options.height != height
            || options.antialiasingSamples != antialiasingPrev)
        {
            width = options.width;
            height = options.height;
            antialiasingPrev = std::max(std::min(options.antialiasingSamples,
            maxAntialiasingSamples), 1u);

    #endif

        vars.textureTargetType
            = antialiasingPrev > 1 ? GL_TEXTURE_2D_MULTISAMPLE
            : GL_TEXTURE_2D;

#ifdef __EMSCRIPTEN__
        static const bool forceSeparateSampleTextures = true;
#else
        static const bool forceSeparateSampleTextures = false;
#endif

        // delete old textures
        glDeleteTextures(1, &vars.depthReadTexId);
        if (vars.depthRenderTexId != vars.depthReadTexId)
            glDeleteTextures(1, &vars.depthRenderTexId);
        glDeleteTextures(1, &vars.colorRenderTexId);
        if (vars.colorRenderTexId != vars.colorReadTexId)
            glDeleteTextures(1, &vars.colorRenderTexId);
        vars.depthReadTexId = vars.depthRenderTexId = 0;
        vars.colorReadTexId = vars.colorRenderTexId = 0;

        #ifdef FILTERS
            glDeleteTextures(1, &colorRender2TexId);
            colorRender2TexId = 0;

            #ifdef SSAO
                glDeleteTextures(1, &normalRenderTexId);
                normalRenderTexId = 0;
            #endif

        #endif

        // depth texture for rendering
        glActiveTexture(GL_TEXTURE0 + 5);
        glGenTextures(1, &vars.depthRenderTexId);
        glBindTexture(vars.textureTargetType, vars.depthRenderTexId);
        if (GLAD_GL_KHR_debug)
        {
            glObjectLabel(GL_TEXTURE, vars.depthRenderTexId,
                -1, "depthRenderTexId");
        }
        if (antialiasingPrev > 1)
        {
            glTexImage2DMultisample(vars.textureTargetType,
                antialiasingPrev, GL_DEPTH24_STENCIL8,
                options.width, options.height, GL_TRUE);
        }
        else
        {
            glTexImage2D(vars.textureTargetType, 0, GL_DEPTH24_STENCIL8,
                options.width, options.height,
                0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
            glTexParameteri(vars.textureTargetType,
                GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(vars.textureTargetType,
                GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
        CHECK_GL("update depth texture for rendering");

        // depth texture for sampling
        glActiveTexture(GL_TEXTURE0 + 6);
        if (antialiasingPrev > 1 || forceSeparateSampleTextures)
        {
            glGenTextures(1, &vars.depthReadTexId);
            glBindTexture(GL_TEXTURE_2D, vars.depthReadTexId);
            if (GLAD_GL_KHR_debug)
            {
                glObjectLabel(GL_TEXTURE, vars.depthReadTexId,
                    -1, "depthReadTexId");
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
                options.width, options.height,
                0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                GL_NEAREST);
        }
        else
        {
            vars.depthReadTexId = vars.depthRenderTexId;
            glBindTexture(GL_TEXTURE_2D, vars.depthReadTexId);
        }
        CHECK_GL("update depth texture for sampling");

        // color texture for rendering
        glActiveTexture(GL_TEXTURE0 + 7);
        glGenTextures(1, &vars.colorRenderTexId);
        glBindTexture(vars.textureTargetType, vars.colorRenderTexId);
        if (GLAD_GL_KHR_debug)
        {
            glObjectLabel(GL_TEXTURE, vars.colorRenderTexId,
                -1, "colorRenderTexId");
        }
        if (antialiasingPrev > 1)
        {
            glTexImage2DMultisample(
                vars.textureTargetType, antialiasingPrev, GL_RGB8,
                options.width, options.height, GL_TRUE);
        }
        else
        {
            glTexImage2D(vars.textureTargetType, 0, GL_RGB8,
                options.width, options.height,
                0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(vars.textureTargetType,
                GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(vars.textureTargetType,
                GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
        CHECK_GL("update color texture for rendering");

        // color texture for sampling
        glActiveTexture(GL_TEXTURE0 + 8);
        if (antialiasingPrev > 1 || forceSeparateSampleTextures)
        {
            glGenTextures(1, &vars.colorReadTexId);
            glBindTexture(GL_TEXTURE_2D, vars.colorReadTexId);
            if (GLAD_GL_KHR_debug)
            {
                glObjectLabel(GL_TEXTURE, vars.colorReadTexId,
                    -1, "colorReadTexId");
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                options.width, options.height,
                0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                GL_NEAREST);
        }
        else
        {
            vars.colorReadTexId = vars.colorRenderTexId;
            glBindTexture(GL_TEXTURE_2D, vars.colorReadTexId);
        }

        #ifdef FILTERS
            // color texture for postprocessing
            if (filtersUsed)
            {
                glActiveTexture(GL_TEXTURE0 + 9);

                glGenTextures(1, &colorRender2TexId);
                glBindTexture(GL_TEXTURE_2D, colorRender2TexId);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                    options.width, options.height,
                    0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);

                #ifdef SSAO

                    glActiveTexture(GL_TEXTURE0 + 10);

                    glGenTextures(1, &normalRenderTexId);
                    glBindTexture(GL_TEXTURE_2D, normalRenderTexId);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                        options.width, options.height,
                        0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                        GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_NEAREST);

                #endif

            }
        #endif

        if (GLAD_GL_KHR_debug)
            glObjectLabel(GL_TEXTURE, vars.colorReadTexId,
                -1, "colorReadTexId");

        CHECK_GL("update color texture for sampling");

        // render frame buffer
        glDeleteFramebuffers(1, &vars.frameRenderBufferId);
        glGenFramebuffers(1, &vars.frameRenderBufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
        if (GLAD_GL_KHR_debug)
        {
            glObjectLabel(GL_FRAMEBUFFER, vars.frameRenderBufferId,
                -1, "frameRenderBufferId");
        }
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
            vars.textureTargetType, vars.depthRenderTexId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            vars.textureTargetType, vars.colorRenderTexId, 0);

        #ifdef FILTERS

            #ifdef SSAO

                if (filtersUsed)
                {
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                        vars.textureTargetType, normalRenderTexId, 0);

                    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0,
                        GL_COLOR_ATTACHMENT1 };

                    glDrawBuffers(2, attachments);
                }

            #endif

        #endif


        checkGlFramebuffer(GL_FRAMEBUFFER);

        // sample frame buffer
        glDeleteFramebuffers(1, &vars.frameReadBufferId);
        glGenFramebuffers(1, &vars.frameReadBufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameReadBufferId);
        if (GLAD_GL_KHR_debug)
        {
            glObjectLabel(GL_FRAMEBUFFER, vars.frameReadBufferId,
                -1, "frameReadBufferId");
        }
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
            GL_TEXTURE_2D, vars.depthReadTexId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, vars.colorReadTexId, 0);
        checkGlFramebuffer(GL_FRAMEBUFFER);

        glActiveTexture(GL_TEXTURE0);
        CHECK_GL("update frame buffer");

        #ifdef FILTERS
            if (filtersUsed)
            {
                // render frame buffer2
                glDeleteFramebuffers(1, &frameRender2BufferId);
                glGenFramebuffers(1, &frameRender2BufferId);
                glBindFramebuffer(GL_FRAMEBUFFER, frameRender2BufferId);
                if (GLAD_GL_KHR_debug)
                    glObjectLabel(GL_FRAMEBUFFER, frameRender2BufferId,
                        -1, "frameRender2BufferId");
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    vars.textureTargetType, colorRender2TexId, 0);
                checkGlFramebuffer(GL_FRAMEBUFFER);
            }
        #endif
    }
}

void inverseMat4(float *a, float *b) {
    float c = a[0],
        d = a[1],
        e = a[2],
        g = a[3],
        f = a[4],
        h = a[5],
        i = a[6],
        j = a[7],
        k = a[8],
        l = a[9],
        o = a[10],
        m = a[11],
        n = a[12],
        p = a[13],
        r = a[14],
        s = a[15],
        A = c * h - d * f,
        B = c * i - e * f,
        t = c * j - g * f,
        u = d * i - e * h,
        v = d * j - g * h,
        w = e * j - g * i,
        x = k * p - l * n,
        y = k * r - o * n,
        z = k * s - m * n,
        C = l * r - o * p,
        D = l * s - m * p,
        E = o * s - m * r,
        q = 1 / (A * E - B * D + t * C + u * z - v * y + w * x);
    b[0] = (h * E - i * D + j * C) * q;
    b[1] = (-d * E + e * D - g * C) * q;
    b[2] = (p * w - r * v + s * u) * q;
    b[3] = (-l * w + o * v - m * u) * q;
    b[4] = (-f * E + i * z - j * y) * q;
    b[5] = (c * E - e * z + g * y) * q;
    b[6] = (-n * w + r * t - s * B) * q;
    b[7] = (k * w - o * t + m * B) * q;
    b[8] = (f * D - h * z + j * x) * q;
    b[9] = (-c * D + d * z - g * x) * q;
    b[10] = (n * v - p * t + s * A) * q;
    b[11] = (-k * v + l * t - m * A) * q;
    b[12] = (-f * C + h * y - i * x) * q;
    b[13] = (c * C - d * y + e * x) * q;
    b[14] = (-n * u + p * B - r * A) * q;
    b[15] = (k * u - l * B + o * A) * q;
}

void RenderViewImpl::renderValid()
{
    OPTICK_EVENT();

    viewInv = view.inverse();
    projInv = proj.inverse();
    viewProj = proj * view;
    viewProjInv = viewProj.inverse();

    auto shaderSurface = context->shaderSurface;

    #ifdef FILTERS
        #ifdef SSAO
            if (options.filterFX == 3 || options.filterSSAO > 0)
                shaderSurface = context->shaderSurfaceWithNormalTexure;
        #endif
    #endif

    // update atmosphere
    updateAtmosphereBuffer();

    // render opaque
    if (!draws->opaque.empty())
    {
        OPTICK_EVENT("opaque");
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        shaderSurface->bind();
        enableClipDistance(true);
        for (const DrawSurfaceTask &t : draws->opaque)
            drawSurface(t);
        enableClipDistance(false);
        CHECK_GL("rendered opaque");
    }

    // render background (atmosphere)
    if (options.renderAtmosphere)
    {
        OPTICK_EVENT("background");
        // corner directions
        vec3 camPos = rawToVec3(draws->camera.eye) / body->majorRadius;
        mat4 inv = (viewProj * scaleMatrix(body->majorRadius)).inverse();
        vec4 cornerDirsD[4] = {
            inv * vec4(-1, -1, 0, 1),
            inv * vec4(+1, -1, 0, 1),
            inv * vec4(-1, +1, 0, 1),
            inv * vec4(+1, +1, 0, 1)
        };
        vec3f cornerDirs[4];
        for (uint32 i = 0; i < 4; i++)
            cornerDirs[i] = normalize(vec3(vec4to3(cornerDirsD[i], true)
                - camPos)).cast<float>();

        context->shaderBackground->bind();
        for (uint32 i = 0; i < 4; i++)
            context->shaderBackground->uniformVec3(i, cornerDirs[i].data());
        context->meshQuad->bind();
        context->meshQuad->dispatch();
        CHECK_GL("rendered background");
    }

    // render transparent
    if (!draws->transparent.empty())
    {
        OPTICK_EVENT("transparent");
        glEnable(GL_BLEND);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0, -10);
        glDepthMask(GL_FALSE);
        shaderSurface->bind();
        enableClipDistance(true);
        for (const DrawSurfaceTask &t : draws->transparent)
            drawSurface(t);
        enableClipDistance(false);
        glDepthMask(GL_TRUE);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0, 0);
        CHECK_GL("rendered transparent");
    }

    // render polygon edges
    if (options.renderPolygonEdges)
    {
        OPTICK_EVENT("polygon_edges");
        glDisable(GL_BLEND);
#ifndef __EMSCRIPTEN__
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glPolygonOffset(0, -1000);
#ifndef VTSR_OPENGLES
        glEnable(GL_POLYGON_OFFSET_LINE);
#endif
#endif
        shaderSurface->bind();
        enableClipDistance(true);
        for (const DrawSurfaceTask &it : draws->opaque)
        {
            DrawSurfaceTask t(it);
            t.flatShading = false;
            t.color[0] = t.color[1] = t.color[2] = t.color[3] = 0;
#ifdef __EMSCRIPTEN__
            drawSurface(t, true);
#else
            drawSurface(t);
#endif
        }
        enableClipDistance(false);
#ifndef __EMSCRIPTEN__
#ifndef VTSR_OPENGLES
        glDisable(GL_POLYGON_OFFSET_LINE);
#endif
        glPolygonOffset(0, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
        glEnable(GL_BLEND);
        CHECK_GL("rendered polygon edges");
    }

    // copy the depth (resolve multisampling)
    if (vars.depthReadTexId != vars.depthRenderTexId)
    {
        OPTICK_EVENT("copy_depth_resolve_multisampling");
        glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vars.frameReadBufferId);
        CHECK_GL_FRAMEBUFFER(GL_READ_FRAMEBUFFER);
        CHECK_GL_FRAMEBUFFER(GL_DRAW_FRAMEBUFFER);
        glBlitFramebuffer(0, 0, options.width, options.height,
            0, 0, options.width, options.height,
            GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
        CHECK_GL("copied the depth (resolved multisampling)");
    }

    // copy the depth for future use
    {
        OPTICK_EVENT("copy_depth_to_cpu");
        clearGlState();
        if ((frameIndex % 2) == 1)
        {
            uint32 dw = width;
            uint32 dh = height;
            if (!options.debugDepthFeedback)
                dw = dh = 0;
            depthBuffer.performCopy(vars.depthReadTexId, dw, dh, viewProj);
        }
        glViewport(0, 0, options.width, options.height);
        glScissor(0, 0, options.width, options.height);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        CHECK_GL("copy depth");
    }

    // render geodata
    renderGeodata();
    CHECK_GL("rendered geodata");

    // render infographics
    if (!draws->infographics.empty())
    {
        OPTICK_EVENT("infographics");
        glDisable(GL_DEPTH_TEST);
        context->shaderInfographics->bind();
        for (const DrawInfographicsTask &t : draws->infographics)
            drawInfographics(t);
        CHECK_GL("rendered infographics");
    }

    //render colorRender to colorRender2
    #ifdef FILTERS

        if (options.filterAA || options.filterDOF || options.filterSSAO || options.filterFX)
        {
            OPTICK_EVENT("postprocssing");

            switch (options.filterAA)
            {
            case 2: //fxaa
                applyFilter(context->shaderFXAA); break;
            case 3: //fxaa2
                applyFilter(context->shaderFXAA2); break;
            }

            switch (options.filterSSAO)
            {
            case 1: //ssao
            case 2: //ssao2

                std::shared_ptr<Shader> shader;

                shader = context->shaderSSAO;

                shader->bind();

                uint32 id = shader->getId();

                if (options.filterSSAO == 1)
                {
                    float kernel[] = {
                        0.09639892989990391,0.011051308155595423,0.02418914843898985,
                        0.020641662823344124,0.09764174613374559,0.014716143993480132,
                        -0.024417282446416676,-0.059530372497734484,0.08108893689865168,
                        0.03501696367305337,0.0681277531867567,0.07600673208830802,
                        0.05785928290241025,-0.004971065806250804,0.09817253074571715,
                        -0.11899107646112957,-0.023027855871862225,0.013717523437086777,
                        0.08958635963846062,-0.050808186165088004,0.08198820973611252,
                        -0.05504431938060061,0.12518255968880043,0.04204100677881524,
                        0.024617957727216814,-0.019320092580691985,0.1530841359514906,
                        -0.03173292804208657,0.15799677076037544,0.057762784567761793,
                        0.023276690625676,-0.1140349424086619,0.14750293063617534,
                        -0.09687533207852488,0.07930752058179572,0.16402695648493798,
                        -0.17784943477890594,-0.12210813996924401,0.0692079988753119,
                        -0.17953710796924113,-0.13804335463380663,0.10237276491185565,
                        0.23076771277268954,0.03044736026771701,0.14123665087727738,
                        0.06177982276926081,0.2771815862932807,0.08950424797557432,
                        0.19034802067190856,0.15724000971117577,0.21134855185762697,
                        0.20364023345091672,0.13416936856531791,0.2566086543729553,
                        -0.29241340413908784,0.17923431367171486,0.174396238675209,
                        -0.29483203678521763,-0.04079101062449529,0.2924672035697324,
                        -0.05645094893468011,-0.23270143363795284,0.38284726008352926,
                        -0.21316828596121803,0.38339094392331396,0.21288997242340915,
                        0.37965641790621435,-0.3448870255262425,0.11379478373007058,
                        0.49017945579170713,0.27892022692613,0.03296059202971882,
                        -0.3028703551571703,0.0029488727492969456,0.5251665589281918,
                        -0.2147694114876527,0.2059274427293489,0.5771306469471303,
                        -0.19316143263158822,0.11757749302641028,0.6562738767867631,
                        0.24244161434622347,0.22352314841582668,0.6632718290697481,
                        -0.3672439272686124,-0.5787334049314298,0.39092092909826415,
                        0.15918144017524632,-0.7700202942154998,0.29312076596094977,
                        -0.574506758600714,0.3018269024473333,0.6105336594122308,
                        0.1808196575338361,-0.7418764325734027,0.556100153537633
                    };

                    glUniform3fv(glGetUniformLocation(id, "kernel"), 32, kernel);

                    float rotationNoiseTexture[] = {
                        0.28821927309036255,
                        -0.21094180643558502,
                        -0.13540157675743103,
                        -0.27949410676956177,
                        -0.5253652334213257,
                        0.34182244539260864,
                        0.43322160840034485,
                        -0.17836587131023407,
                        0.0551881268620491,
                        -0.6447020769119263,
                        -0.4608458876609802,
                        -0.16228044033050537,
                        -0.33945947885513306,
                        -0.17424403131008148,
                        -0.5142955780029297,
                        -0.5824607014656067
                    };

                    glUniform1fv(glGetUniformLocation(id, "tNoise"), 16, kernel);

                    glUniform1f(glGetUniformLocation(id, "mixFactor"), options.filterSSAOOnly ? 1.0 : 0.0);
                    glUniform1f(glGetUniformLocation(id, "kernelRadius"), options.filterSSAORadius);
                    glUniform1f(glGetUniformLocation(id, "minDistance"), options.filterSSAOMin);
                    glUniform1f(glGetUniformLocation(id, "maxDistance"), options.filterSSAOMax);
                }

                if (options.filterSSAO == 2)
                {
                    glUniform1f(glGetUniformLocation(id, "kernelRadius"), options.filterSSAORadius);

                    /*float scale;
                    float intensity;
                    float bias;
                    float minResolution;
                    vec2 size;
                    float randomSeed;*/
                }

                double cnear, cfar;
                double eye[3], target[3], up[3];

                camera->getView(eye, target, up);
                camera->suggestedNearFar(cnear, cfar);

                glUniform1f(glGetUniformLocation(id, "cameraNear"), cnear);
                glUniform1f(glGetUniformLocation(id, "cameraFar"), cfar);

                //uniform mat4 cameraProjectionMatrix;
                //uniform mat4 cameraInverseProjectionMatrix;

                mat4f cproj = proj.cast<float>();
                //camera->getProj(proj);

                float cinvProj[16];
                inverseMat4(cproj.data(), cinvProj);

                glUniformMatrix4fv(glGetUniformLocation(id, "cameraProjectionMatrix"), 1, GL_FALSE, cproj.data());
                glUniformMatrix4fv(glGetUniformLocation(id, "cameraInverseProjectionMatrix"), 1, GL_FALSE, cinvProj);


                if (options.filterSSAO == 1)
                    applyFilter(context->shaderSSAO);
                else 
                    applyFilter(context->shaderSSAO2);
            }

            switch (options.filterDOF)
            {
            case 1: //dof
            case 2: //dof2

                double cnear, cfar;
                double eye[3], target[3], up[3];

                camera->getView(eye, target, up);
                camera->suggestedNearFar(cnear, cfar);

                vec3 delta = rawToVec3(target) - rawToVec3(eye);
                double distance = sqrt(delta[0]*delta[0] + delta[1] * delta[1] + delta[2] * delta[2]);

                std::shared_ptr<Shader> shader;

                if (options.filterDOF == 1)
                    shader = context->shaderDOF;
                else 
                    shader = context->shaderDOF2;

                shader->bind();

                uint32 id = shader->getId();
                glUniform1f(glGetUniformLocation(id, "maxblur"), options.filterDOFBlur); // 10);

                   
                if (!options.filterDOFFocus)
                {
                    glUniform1f(glGetUniformLocation(id, "focus"), distance);
                    glUniform1f(glGetUniformLocation(id, "aperture"), 1 / (distance * powf(1.4, options.filterDOFAperture)));
                }
                else
                {
                    glUniform1f(glGetUniformLocation(id, "focus"), 50);
                    glUniform1f(glGetUniformLocation(id, "aperture"), 1 / ((distance + 200) * powf(1.4, options.filterDOFAperture)));
                }

                glUniform1f(glGetUniformLocation(id, "nearClip"), cnear);
                glUniform1f(glGetUniformLocation(id, "farClip"), cfar);
                glUniform1f(glGetUniformLocation(id, "aspect"), 1);

                if (options.filterDOF == 1)
                    applyFilter(shader);
                else {
                    glUniform1f(glGetUniformLocation(id, "vpass"), 1);
                    applyFilter(shader);
                    glUniform1f(glGetUniformLocation(id, "vpass"), 0);
                    applyFilter(shader);
                }

                break;
            }

            switch (options.filterFX)
            {
            case 1: //greyscale
                applyFilter(context->shaderGreyscale); break;
            case 2: //depth
                applyFilter(context->shaderDepth); break;
            case 3: //normals
                applyFilter(context->shaderNormal); break;
            }

            CHECK_GL("rendered postprocessing");
        }
    #endif
}

void RenderViewImpl::renderEntry()
{
    CHECK_GL("pre-frame check");
    uboCacheLarge.frame();
    uboCacheSmall.frame();
    clearGlState();
    frameIndex++;

    assert(context->shaderSurface);
    view = rawToMat4(draws->camera.view);
    proj = rawToMat4(draws->camera.proj);

    if (options.width <= 0 || options.height <= 0)
        return;

    updateFramebuffers();

    // initialize opengl
    glViewport(0, 0, options.width, options.height);
    glScissor(0, 0, options.width, options.height);

    glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);

    CHECK_GL_FRAMEBUFFER(GL_FRAMEBUFFER);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT
        | GL_STENCIL_BUFFER_BIT);
    CHECK_GL("initialized opengl state");

    // render
    if (proj(0, 0) != 0)
        renderValid();
    else
        hysteresisJobs.clear();

    // copy the color to output texture
    if (options.colorToTexture
        && vars.colorReadTexId != vars.colorRenderTexId)
    {
        OPTICK_EVENT("colorToTexture");
        glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vars.frameReadBufferId);
        CHECK_GL_FRAMEBUFFER(GL_READ_FRAMEBUFFER);
        CHECK_GL_FRAMEBUFFER(GL_DRAW_FRAMEBUFFER);
        glBlitFramebuffer(0, 0, options.width, options.height,
            0, 0, options.width, options.height,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
        CHECK_GL("copied the color to texture");
    }

    // copy the color to target frame buffer
    if (options.colorToTargetFrameBuffer)
    {
        OPTICK_EVENT("colorToTargetFrameBuffer");
        uint32 w = options.targetViewportW ? options.targetViewportW
            : options.width;
        uint32 h = options.targetViewportH ? options.targetViewportH
            : options.height;
        bool same = w == options.width && h == options.height;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, options.targetFrameBuffer);
        CHECK_GL_FRAMEBUFFER(GL_READ_FRAMEBUFFER);
        CHECK_GL_FRAMEBUFFER(GL_DRAW_FRAMEBUFFER);
        glBlitFramebuffer(0, 0, options.width, options.height,
            options.targetViewportX, options.targetViewportY,
            options.targetViewportX + w, options.targetViewportY + h,
            GL_COLOR_BUFFER_BIT, same ? GL_NEAREST : GL_LINEAR);
        CHECK_GL("copied the color to target frame buffer");
    }

    // clear the state
    clearGlState();

    checkGlImpl("frame end (unconditional check)");
}

void RenderViewImpl::updateAtmosphereBuffer()
{
    OPTICK_EVENT();

    ShaderAtm::AtmBlock atmBlock;

    if (options.renderAtmosphere
        && !projected
        && atmosphereDensityTexture)
    {
        // bind atmosphere density texture
        {
            glActiveTexture(GL_TEXTURE4);
            atmosphereDensityTexture->bind();
            glActiveTexture(GL_TEXTURE0);
        }

        double boundaryThickness, horizontalExponent, verticalExponent;
        atmosphereDerivedAttributes(*body, boundaryThickness,
            horizontalExponent, verticalExponent);

        // uniParams
        atmBlock.uniAtmSizes =
        {
            (float)(boundaryThickness / body->majorRadius),
            (float)(body->majorRadius / body->minorRadius),
            (float)(1.0 / body->majorRadius),
            0.f
        };
        atmBlock.uniAtmCoefs =
        {
            (float)(horizontalExponent),
            (float)(body->atmosphere.colorGradientExponent),
            0.f,
            0.f
        };

        // camera position
        vec3 camPos = rawToVec3(draws->camera.eye) / body->majorRadius;
        atmBlock.uniAtmCameraPosition = camPos.cast<float>();

        // view inv
        mat4 vi = rawToMat4(draws->camera.view).inverse();
        atmBlock.uniAtmViewInv = vi.cast<float>();

        // colors
        atmBlock.uniAtmColorHorizon
            = rawToVec4(body->atmosphere.colorHorizon);
        atmBlock.uniAtmColorZenith
            = rawToVec4(body->atmosphere.colorZenith);
    }
    else
    {
        memset(&atmBlock, 0, sizeof(atmBlock));
    }

    useDisposableUbo(0, atmBlock)->setDebugId("uboAtm");
}

void RenderViewImpl::getWorldPosition(const double screenPos[2],
    double worldPos[3])
{
    vecToRaw(nan3(), worldPos);
    double x = screenPos[0];
    double y = screenPos[1];
    y = height - y - 1;
    x = x / width * 2 - 1;
    y = y / height * 2 - 1;
    double z = depthBuffer.value(x, y) * 2 - 1;
    vecToRaw(vec4to3(vec4(viewProjInv
        * vec4(x, y, z, 1)), true), worldPos);
}

void RenderViewImpl::applyFilter(const std::shared_ptr<Shader> &shader)
{
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameRender2BufferId);
    CHECK_GL_FRAMEBUFFER(GL_READ_FRAMEBUFFER);
    CHECK_GL_FRAMEBUFFER(GL_DRAW_FRAMEBUFFER);

    glBlitFramebuffer(0, 0, options.width, options.height,
        0, 0, options.width, options.height,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
    CHECK_GL_FRAMEBUFFER(GL_FRAMEBUFFER);

    shader->bind();

    glActiveTexture(GL_TEXTURE0 + 9);
    glBindTexture(GL_TEXTURE_2D, colorRender2TexId);

    glActiveTexture(GL_TEXTURE0 + 10);
    glBindTexture(GL_TEXTURE_2D, normalRenderTexId);

    context->meshQuad->bind();
    context->meshQuad->dispatch();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void RenderViewImpl::renderCompass(const double screenPosSize[3],
    const double mapRotation[3])
{
    glEnable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    context->texCompas->bind();
    context->shaderTexture->bind();
    mat4 p = orthographicMatrix(-1, 1, -1, 1, -1, 1)
        * scaleMatrix(1.0 / width, 1.0 / height, 1);
    mat4 v = translationMatrix(screenPosSize[0] * 2 - width,
        screenPosSize[1] * 2 - height, 0)
        * scaleMatrix(screenPosSize[2], screenPosSize[2], 1);
    mat4 m = rotationMatrix(0, mapRotation[1] + 90)
        * rotationMatrix(2, mapRotation[0]);
    mat4f mvpf = (p * v * m).cast<float>();
    mat3f uvmf = identityMatrix3().cast<float>();
    context->shaderTexture->uniformMat4(0, mvpf.data());
    context->shaderTexture->uniformMat3(1, uvmf.data());
    context->meshQuad->bind();
    context->meshQuad->dispatch();
}

} } // namespace vts renderer

