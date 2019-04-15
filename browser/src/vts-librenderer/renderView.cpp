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

#include "renderer.hpp"

#include <vts-browser/resources.hpp>
#include <vts-browser/cameraDraws.hpp>
#include <vts-browser/celestial.hpp>

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
    checkGlImpl("cleared gl state");
}

void enableClipDistance(bool enable)
{
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
    width(0),
    height(0),
    antialiasingPrev(0),
    elapsedTime(0),
    projected(false)
{
    uboAtm = std::make_shared<UniformBuffer>();
    uboAtm->debugId = "uboAtm";
    uboGeodataCamera = std::make_shared<UniformBuffer>();
    uboGeodataCamera->debugId = "uboGeodataCamera";
    depthBuffer.meshQuad = context->meshQuad;
    depthBuffer.shaderCopyDepth = context->shaderCopyDepth;
}

void RenderViewImpl::drawSurface(const DrawSurfaceTask &t)
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
        mat3x4f uvMat;
        vec4f uvClip;
        vec4f color;
        vec4si32 flags; // mask, monochromatic, flat shading, uv source
    } uboSurface;

    uboSurface.p = proj.cast<float>();
    uboSurface.mv = rawToMat4(t.mv);
    uboSurface.uvMat = mat3x4f(rawToMat3(t.uvm));
    uboSurface.uvClip = rawToVec4(t.uvClip);
    uboSurface.color = rawToVec4(t.color);
    uboSurface.flags = vec4si32(
                t.texMask ? 1 : -1,
                tex->getGrayscale() ? 1 : -1,
                t.flatShading ? 1 : -1,
                t.externalUv ? 1 : -1
                );

    auto ubo = std::make_shared<UniformBuffer>();
    ubo->debugId = "UboSurface";
    ubo->bind();
    ubo->load(uboSurface);
    ubo->bindToIndex(1);

    if (t.texMask)
    {
        glActiveTexture(GL_TEXTURE0 + 1);
        ((Texture*)t.texMask.get())->bind();
        glActiveTexture(GL_TEXTURE0 + 0);
    }
    tex->bind();

    m->bind();
    m->dispatch();
}

void RenderViewImpl::drawInfographic(const DrawSimpleTask &t)
{
    Mesh *m = (Mesh*)t.mesh.get();
    if (!m)
        return;

    struct UboInfographics
    {
        mat4f mvp;
        vec4f color;
        vec4si32 useColorTexture;
    } uboInfographics;

    uboInfographics.mvp = proj.cast<float>() * rawToMat4(t.mv);
    uboInfographics.color = rawToVec4(t.color);
    uboInfographics.useColorTexture[0] = !!t.texColor;

    auto ubo = std::make_shared<UniformBuffer>();
    ubo->debugId = "UboInfographics";
    ubo->bind();
    ubo->load(uboInfographics);
    ubo->bindToIndex(1);

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
    if (options.width != width || options.height != height
        || options.antialiasingSamples != antialiasingPrev)
    {
        width = options.width;
        height = options.height;
        antialiasingPrev = std::max(std::min(options.antialiasingSamples,
            maxAntialiasingSamples), 1u);

        vars.textureTargetType
            = antialiasingPrev > 1 ? GL_TEXTURE_2D_MULTISAMPLE
            : GL_TEXTURE_2D;

        // delete old textures
        glDeleteTextures(1, &vars.depthReadTexId);
        if (vars.depthRenderTexId != vars.depthReadTexId)
            glDeleteTextures(1, &vars.depthRenderTexId);
        glDeleteTextures(1, &vars.colorRenderTexId);
        if (vars.colorRenderTexId != vars.colorReadTexId)
            glDeleteTextures(1, &vars.colorRenderTexId);
        vars.depthReadTexId = vars.depthRenderTexId = 0;
        vars.colorReadTexId = vars.colorRenderTexId = 0;

        // depth texture for rendering
        glActiveTexture(GL_TEXTURE0 + 5);
        glGenTextures(1, &vars.depthRenderTexId);
        glBindTexture(vars.textureTargetType, vars.depthRenderTexId);
        if (GLAD_GL_KHR_debug)
            glObjectLabel(GL_TEXTURE, vars.depthRenderTexId,
                -1, "depthRenderTexId");
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
        if (antialiasingPrev > 1)
        {
            glGenTextures(1, &vars.depthReadTexId);
            glBindTexture(GL_TEXTURE_2D, vars.depthReadTexId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
                options.width, options.height,
                0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
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
        if (GLAD_GL_KHR_debug)
            glObjectLabel(GL_TEXTURE, vars.depthReadTexId,
                -1, "depthReadTexId");
        CHECK_GL("update depth texture for sampling");

        // color texture for rendering
        glActiveTexture(GL_TEXTURE0 + 7);
        glGenTextures(1, &vars.colorRenderTexId);
        glBindTexture(vars.textureTargetType, vars.colorRenderTexId);
        if (GLAD_GL_KHR_debug)
            glObjectLabel(GL_TEXTURE, vars.colorRenderTexId,
                -1, "colorRenderTexId");
        if (antialiasingPrev > 1)
            glTexImage2DMultisample(
                vars.textureTargetType, antialiasingPrev, GL_RGB8,
                options.width, options.height, GL_TRUE);
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
        if (antialiasingPrev > 1)
        {
            glGenTextures(1, &vars.colorReadTexId);
            glBindTexture(GL_TEXTURE_2D, vars.colorReadTexId);
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
        if (GLAD_GL_KHR_debug)
            glObjectLabel(GL_TEXTURE, vars.colorReadTexId,
                -1, "colorReadTexId");
        CHECK_GL("update color texture for sampling");

        // render frame buffer
        glDeleteFramebuffers(1, &vars.frameRenderBufferId);
        glGenFramebuffers(1, &vars.frameRenderBufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
        if (GLAD_GL_KHR_debug)
            glObjectLabel(GL_FRAMEBUFFER, vars.frameRenderBufferId,
                -1, "frameRenderBufferId");
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
            vars.textureTargetType, vars.depthRenderTexId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            vars.textureTargetType, vars.colorRenderTexId, 0);
        checkGlFramebuffer(GL_FRAMEBUFFER);

        // sample frame buffer
        glDeleteFramebuffers(1, &vars.frameReadBufferId);
        glGenFramebuffers(1, &vars.frameReadBufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameReadBufferId);
        if (GLAD_GL_KHR_debug)
            glObjectLabel(GL_FRAMEBUFFER, vars.frameReadBufferId,
                -1, "frameReadBufferId");
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D, vars.depthReadTexId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, vars.colorReadTexId, 0);
        checkGlFramebuffer(GL_FRAMEBUFFER);

        CHECK_GL("update frame buffer");
    }
}

void RenderViewImpl::render()
{
    CHECK_GL("pre-frame check");

    assert(context->shaderSurface);
    view = rawToMat4(draws->camera.view);
    proj = rawToMat4(draws->camera.proj);

    if (options.width <= 0 || options.height <= 0
        || proj(0, 0) == 0)
        return;

    viewInv = view.inverse();
    projInv = proj.inverse();
    viewProj = proj * view;
    viewProjInv = viewProj.inverse();

    updateFramebuffers();

    // initialize opengl
    glViewport(0, 0, options.width, options.height);
    glScissor(0, 0, options.width, options.height);
    glActiveTexture(GL_TEXTURE0);
    glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
    CHECK_GL_FRAMEBUFFER(GL_FRAMEBUFFER);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
#ifndef VTSR_OPENGLES
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(0, -1000);
#endif
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT
        | GL_STENCIL_BUFFER_BIT);
    CHECK_GL("initialized opengl");

    // update atmosphere
    updateAtmosphereBuffer();

    // render opaque
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    context->shaderSurface->bind();
    enableClipDistance(true);
    for (const DrawSurfaceTask &t : draws->opaque)
        drawSurface(t);
    enableClipDistance(false);
    CHECK_GL("rendered opaque");

    // render background (atmosphere)
    {
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
    }

    // render transparent
    glEnable(GL_BLEND);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0, -10);
    glDepthMask(GL_FALSE);
    context->shaderSurface->bind();
    enableClipDistance(true);
    for (const DrawSurfaceTask &t : draws->transparent)
        drawSurface(t);
    enableClipDistance(false);
    glDepthMask(GL_TRUE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    CHECK_GL("rendered transparent");

    // render polygon edges
    if (options.renderPolygonEdges)
    {
        glDisable(GL_BLEND);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        context->shaderSurface->bind();
        enableClipDistance(true);
        for (const DrawSurfaceTask &it : draws->opaque)
        {
            DrawSurfaceTask t(it);
            t.flatShading = false;
            t.color[0] = t.color[1] = t.color[2] = t.color[3] = 0;
            drawSurface(t);
        }
        enableClipDistance(false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_BLEND);
        CHECK_GL("rendered polygon edges");
    }

    // copy the depth (resolve multisampling)
    if (vars.depthReadTexId != vars.depthRenderTexId)
    {
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
    clearGlState();
    depthBuffer.performCopy(vars.depthReadTexId, width, height, viewProj);
    glViewport(0, 0, options.width, options.height);
    glScissor(0, 0, options.width, options.height);
    glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    // render geodata
    renderGeodata();
    CHECK_GL("rendered geodata");

    // render infographics
    glDisable(GL_DEPTH_TEST);
    context->shaderInfographic->bind();
    for (const DrawSimpleTask &t : draws->infographics)
        drawInfographic(t);
    CHECK_GL("rendered infographics");

    // copy the color (resolve multisampling)
    if (options.colorToTexture
        && vars.colorReadTexId != vars.colorRenderTexId)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vars.frameReadBufferId);
        CHECK_GL_FRAMEBUFFER(GL_READ_FRAMEBUFFER);
        CHECK_GL_FRAMEBUFFER(GL_DRAW_FRAMEBUFFER);
        glBlitFramebuffer(0, 0, options.width, options.height,
            0, 0, options.width, options.height,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
        CHECK_GL("copied the color to texture (resolving multisampling)");
    }

    // copy the color to screen
    if (options.colorToTargetFrameBuffer)
    {
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
        CHECK_GL("copied the color to screen (resolving multisampling)");
    }

    // clear the state
    clearGlState();
}

void RenderViewImpl::updateAtmosphereBuffer()
{
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

    // Load it in
    uboAtm->bind();
    uboAtm->load(atmBlock);

    // Bind atmosphere uniform buffer to index 0
    uboAtm->bindToIndex(0);
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

