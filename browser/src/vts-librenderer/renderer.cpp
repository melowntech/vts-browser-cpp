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

class ShaderAtm : public Shader
{
public:
    struct AtmBlock
    {
        mat4f uniAtmViewInv;
        vec4f uniAtmColorHorizon;
        vec4f uniAtmColorZenith;
        vec4f uniAtmSizes; // atmosphere thickness (divided by major axis), major / minor axes ratio, inverze major axis
        vec4f uniAtmCoefs; // horizontal exponent, colorGradientExponent
        vec3f uniAtmCameraPosition; // world position of camera (divided by major axis)
    };

    void initializeAtmosphere()
    {
        bindUniformBlockLocations({ {"uboAtm", 0} });
        bindTextureLocations({ {"texAtmDensity", 4} });
    }
};

void RendererImpl::clearGlState()
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
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glPolygonOffset(0, 0);
    checkGlImpl("cleared gl state");
}

RendererImpl::RendererImpl(Renderer *rendererApi)
    : rendererApi(rendererApi), draws(nullptr), body(nullptr),
    atmosphereDensityTexture(nullptr),
    widthPrev(0), heightPrev(0), antialiasingPrev(0),
    projected(false)
{}

RendererImpl::~RendererImpl()
{}

void RendererImpl::drawSurface(const DrawSurfaceTask &t)
{
    Texture *tex = (Texture*)t.texColor.get();
    Mesh *m = (Mesh*)t.mesh.get();
    if (!m || !tex)
        return;
    shaderSurface->uniformMat4(1, t.mv);
    shaderSurface->uniformMat3(2, t.uvm);
    shaderSurface->uniformVec4(3, t.color);
    shaderSurface->uniformVec4(4, t.uvClip);
    int flags[4] = {
        t.texMask ? 1 : -1,
        tex->getGrayscale() ? 1 : -1,
        t.flatShading ? 1 : -1,
        t.externalUv ? 1 : -1
    };
    shaderSurface->uniformVec4(5, flags);
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

void RendererImpl::drawInfographic(const DrawSimpleTask &t)
{
    Mesh *m = (Mesh*)t.mesh.get();
    if (!m)
        return;
    shaderInfographic->uniformMat4(1, t.mv);
    shaderInfographic->uniformVec4(2, t.color);
    shaderInfographic->uniform(3, (int)(!!t.texColor));
    if (t.texColor)
    {
        Texture *tex = (Texture*)t.texColor.get();
        tex->bind();
    }
    m->bind();
    m->dispatch();
}

void RendererImpl::enableClipDistance(bool enable)
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

void RendererImpl::updateFramebuffers()
{
    if (options.width != widthPrev || options.height != heightPrev
        || options.antialiasingSamples != antialiasingPrev)
    {
        widthPrev = options.width;
        heightPrev = options.height;
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
                0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
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
        checkGlFramebuffer(GL_FRAMEBUFFER);

        CHECK_GL("update frame buffer");
    }
}

void RendererImpl::render()
{
    CHECK_GL("pre-frame check");

    assert(shaderSurface);
    view = rawToMat4(draws->camera.view);
    proj = rawToMat4(draws->camera.proj);

    if (options.width <= 0 || options.height <= 0
        || proj(0, 0) == 0)
        return;

    viewInv = view.inverse();
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

    // update shaders
    mat4f projf = rawToMat4(draws->camera.proj).cast<float>();
    shaderInfographic->bind();
    shaderInfographic->uniformMat4(0, projf.data());
    shaderSurface->bind();
    shaderSurface->uniformMat4(0, projf.data());

    // render opaque
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    shaderSurface->bind();
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

        shaderBackground->bind();
        for (uint32 i = 0; i < 4; i++)
            shaderBackground->uniformVec3(i, cornerDirs[i].data());
        meshQuad->bind();
        meshQuad->dispatch();
    }

    // render transparent
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
    CHECK_GL("rendered transparent");

    // render polygon edges
    if (options.renderPolygonEdges)
    {
        glDisable(GL_BLEND);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        shaderSurface->bind();
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

    // render geodata
    renderGeodata();
    CHECK_GL("rendered geodata");

    // render infographics
    glDisable(GL_DEPTH_TEST);
    shaderInfographic->bind();
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

void RendererImpl::initialize()
{
    // load texture compas
    {
        texCompas = std::make_shared<Texture>();
        vts::GpuTextureSpec spec(vts::readInternalMemoryBuffer(
            "data/textures/compas.png"));
        spec.verticalFlip();
        vts::ResourceInfo ri;
        texCompas->load(ri, spec, "data/textures/compas.png");
    }

    // load shader texture
    {
        shaderTexture = std::make_shared<Shader>();
        shaderTexture->debugId
            = "data/shaders/texture.*.glsl";
        shaderTexture->loadInternal(
            "data/shaders/texture.vert.glsl",
            "data/shaders/texture.frag.glsl");
        shaderTexture->loadUniformLocations({ "uniMvp", "uniUvm" });
        shaderTexture->bindTextureLocations({ {"uniTexture", 0} });
    }

    // load shader surface
    {
        shaderSurface = std::make_shared<ShaderAtm>();
        shaderSurface->debugId
            = "data/shaders/surface.*.glsl";
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/surface.vert.glsl");
        Buffer atm = readInternalMemoryBuffer(
            "data/shaders/atmosphere.inc.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/surface.frag.glsl");
        shaderSurface->load(vert.str(), atm.str() + frag.str());
        shaderSurface->loadUniformLocations({ "uniP", "uniMv", "uniUvMat",
            "uniColor", "uniUvClip", "uniFlags" });
        shaderSurface->bindTextureLocations({ {"texColor", 0}, {"texMask", 1} });
        shaderSurface->initializeAtmosphere();
    }

    // load shader background
    {
        shaderBackground = std::make_shared<ShaderAtm>();
        shaderBackground->debugId
            = "data/shaders/background.*.glsl";
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/background.vert.glsl");
        Buffer atm = readInternalMemoryBuffer(
            "data/shaders/atmosphere.inc.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/background.frag.glsl");
        shaderBackground->load(vert.str(), atm.str() + frag.str());
        shaderBackground->loadUniformLocations({ "uniCorners[0]",
            "uniCorners[1]","uniCorners[2]","uniCorners[3]" });
        shaderBackground->initializeAtmosphere();
    }

    // load shader infographic
    {
        shaderInfographic = std::make_shared<Shader>();
        shaderInfographic->debugId
            = "data/shaders/infographic.*.glsl";
        shaderInfographic->loadInternal(
            "data/shaders/infographic.vert.glsl",
            "data/shaders/infographic.frag.glsl");
        shaderInfographic->loadUniformLocations({ "uniP", "uniMv", "uniColor",
                                                    "uniUseColorTexture" });
        shaderInfographic->bindTextureLocations({ {"texColor", 0},
                                                    {"texDepth", 6} });
    }

    // load shader copy depth
    {
        shaderCopyDepth = std::make_shared<Shader>();
        shaderCopyDepth->debugId
            = "data/shaders/copyDepth.*.glsl";
        shaderCopyDepth->loadInternal(
            "data/shaders/copyDepth.vert.glsl",
            "data/shaders/copyDepth.frag.glsl");
        shaderCopyDepth->loadUniformLocations({ "uniTexPos" });
        shaderCopyDepth->bindTextureLocations({ {"texDepth", 0} });
    }

    // load mesh quad
    {
        meshQuad = std::make_shared<Mesh>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer(
            "data/meshes/quad.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Triangles);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        spec.attributes[1].enable = true;
        spec.attributes[1].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[1].components = 2;
        spec.attributes[1].offset = sizeof(vts::vec3f);
        vts::ResourceInfo ri;
        meshQuad->load(ri, spec, "data/meshes/quad.obj");
    }

    // load mesh rect
    {
        meshRect = std::make_shared<Mesh>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer(
            "data/meshes/rect.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Triangles);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        spec.attributes[1].enable = true;
        spec.attributes[1].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[1].components = 2;
        spec.attributes[1].offset = sizeof(vts::vec3f);
        vts::ResourceInfo ri;
        meshRect->load(ri, spec, "data/meshes/rect.obj");
    }

    // load mesh line
    {
        meshLine = std::make_shared<Mesh>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer(
            "data/meshes/line.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Lines);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        spec.attributes[1].enable = true;
        spec.attributes[1].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[1].components = 2;
        spec.attributes[1].offset = sizeof(vts::vec3f);
        vts::ResourceInfo ri;
        meshLine->load(ri, spec, "data/meshes/line.obj");
    }

    // load mesh empty
    {
        meshEmpty = std::make_shared<Mesh>();
        vts::GpuMeshSpec spec;
        spec.faceMode = vts::GpuMeshSpec::FaceMode::Triangles;
        vts::ResourceInfo ri;
        meshEmpty->load(ri, spec, "meshEmpty");
    }

    // create atmosphere ubo
    {
        uboAtm = std::make_shared<UniformBuffer>();
        uboAtm->debugId = "uboAtm";
    }

    initializeGeodata();
}

void RendererImpl::finalize()
{
    texCompas.reset();
    shaderTexture.reset();
    shaderSurface.reset();
    shaderBackground.reset();
    shaderInfographic.reset();
    shaderCopyDepth.reset();
    shaderGeodataLine.reset();
    shaderGeodataPoint.reset();
    shaderGeodataPointLabel.reset();
    meshQuad.reset();
    meshRect.reset();
    meshLine.reset();
    meshEmpty.reset();
    uboAtm.reset();
    uboGeodataCamera.reset();
    uboGeodataView.reset();

    if (vars.frameRenderBufferId)
    {
        glDeleteFramebuffers(1, &vars.frameRenderBufferId);
        vars.frameRenderBufferId = 0;
    }

    if (vars.frameReadBufferId)
    {
        glDeleteFramebuffers(1, &vars.frameReadBufferId);
        vars.frameReadBufferId = 0;
    }

    if (vars.depthRenderTexId)
    {
        glDeleteTextures(1, &vars.depthRenderTexId);
        vars.depthRenderTexId = 0;
    }

    if (vars.depthReadTexId)
    {
        glDeleteTextures(1, &vars.depthReadTexId);
        vars.depthReadTexId = 0;
    }

    if (vars.colorRenderTexId)
    {
        glDeleteTextures(1, &vars.colorRenderTexId);
        vars.colorRenderTexId = 0;
    }

    widthPrev = heightPrev = antialiasingPrev = 0;
}

void RendererImpl::updateAtmosphereBuffer()
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
            = rawToVec4(body->atmosphere.colorHorizon.data());
        atmBlock.uniAtmColorZenith
            = rawToVec4(body->atmosphere.colorZenith.data());
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

void RendererImpl::renderCompass(const double screenPosSize[3],
    const double mapRotation[3])
{
    glEnable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    texCompas->bind();
    shaderTexture->bind();
    mat4 p = orthographicMatrix(-1, 1, -1, 1, -1, 1)
        * scaleMatrix(1.0 / widthPrev, 1.0 / heightPrev, 1);
    mat4 v = translationMatrix(screenPosSize[0] * 2 - widthPrev,
        screenPosSize[1] * 2 - heightPrev, 0)
        * scaleMatrix(screenPosSize[2], screenPosSize[2], 1);
    mat4 m = rotationMatrix(0, mapRotation[1] + 90)
        * rotationMatrix(2, mapRotation[0]);
    mat4f mvpf = (p * v * m).cast<float>();
    mat3f uvmf = identityMatrix3().cast<float>();
    shaderTexture->uniformMat4(0, mvpf.data());
    shaderTexture->uniformMat3(1, uvmf.data());
    meshQuad->bind();
    meshQuad->dispatch();
}

void RendererImpl::getWorldPosition(const double screenPos[2],
    double worldPos[3])
{
    double x = screenPos[0];
    double y = screenPos[1];
    y = heightPrev - y - 1;

    float depth = std::numeric_limits<float>::quiet_NaN();
#ifdef VTSR_OPENGLES
    // opengl ES does not support reading depth with glReadPixels
    {
        clearGlState();
        uint32 fbId = 0, texId = 0;

        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glGenFramebuffers(1, &fbId);
        glBindFramebuffer(GL_FRAMEBUFFER, fbId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, texId, 0);
        checkGlFramebuffer(GL_FRAMEBUFFER);

        shaderCopyDepth->bind();
        int pos[2] = { (int)x, (int)y };
        shaderCopyDepth->uniformVec2(0, pos);
        glBindTexture(GL_TEXTURE_2D, vars.depthReadTexId);
        meshQuad->bind();
        meshQuad->dispatch();

        unsigned char res[4];
        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, res);
        if (res[0] != 0 || res[1] != 0 || res[2] != 0 || res[3] != 0)
        {
            static const vec4 bitSh = vec4(1.0 / (256.0*256.0*256.0),
                1.0 / (256.0*256.0),
                1.0 / 256.0, 1.0);
            depth = 0;
            for (int i = 0; i < 4; i++)
                depth += res[i] * bitSh[i];
            depth /= 255;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbId);
        glDeleteTextures(1, &texId);
    }
#else
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameReadBufferId);
        glReadPixels((int)x, (int)y, 1, 1,
            GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    }
#endif
    CHECK_GL("read depth");
    clearGlState();

    if (depth > 1 - 1e-7)
        depth = std::numeric_limits<float>::quiet_NaN();
    depth = depth * 2 - 1;
    x = x / widthPrev * 2 - 1;
    y = y / heightPrev * 2 - 1;
    vec3 res = vec4to3(vec4(viewProj.inverse()
            * vec4(x, y, depth, 1)), true);
    for (int i = 0; i < 3; i++)
        worldPos[i] = res[i];
}

} } // namespace vts renderer

