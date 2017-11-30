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

#include <assert.h>

#include <vts-browser/buffer.hpp>
#include <vts-browser/math.hpp>

#include "renderer.hpp"

namespace vts { namespace renderer
{

namespace priv
{

int maxAntialiasingSamples = 1;
float maxAnisotropySamples = 0.f;

} // namespace priv

using namespace priv;

namespace
{

std::shared_ptr<Shader> shaderSurface;
std::shared_ptr<Shader> shaderInfographic;
std::shared_ptr<Shader> shaderAtmosphere;
std::shared_ptr<Shader> shaderCopyDepth;
std::shared_ptr<Mesh> meshQuad; // positions: -1 .. 1
std::shared_ptr<Mesh> meshRect; // positions: 0 .. 1

RenderVariables vars;

int widthPrev;
int heightPrev;
int antialiasingPrev;

mat4 view;
mat4 proj;
mat4 viewProj;
mat4 viewProjInv;

double sqr(double a)
{
    return a * a;
}

void clearGlState()
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkGl("cleared gl state");
}

class Renderer
{
public:
    const RenderOptions &options;
    const MapDraws &draws;
    const MapCelestialBody &body;

    Renderer(const RenderOptions &options,
             const MapDraws &draws,
             const MapCelestialBody &body) :
        options(options), draws(draws), body(body)
    {
        assert(shaderSurface);

        view = rawToMat4(draws.camera.view);
        proj = rawToMat4(draws.camera.proj);
        viewProj = proj * view;
        viewProjInv =  viewProj.inverse();
    }

    void drawSurface(const DrawTask &t)
    {
        Texture *tex = (Texture*)t.texColor.get();
        Mesh *m = (Mesh*)t.mesh.get();
        shaderSurface->bind();
        shaderSurface->uniformMat4(0, t.mvp);
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
        if (t.flatShading)
        {
            mat4f mv = mat4f(t.mvp);
            mv = proj.inverse().cast<float>() * mv;
            shaderSurface->uniformMat4(1, (float*)mv.data());
        }
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

    void drawInfographic(const DrawTask &t)
    {
        shaderInfographic->bind();
        shaderInfographic->uniformMat4(0, t.mvp);
        shaderInfographic->uniformVec4(1, t.color);
        shaderInfographic->uniform(2, (int)(!!t.texColor));
        if (t.texColor)
        {
            Texture *tex = (Texture*)t.texColor.get();
            tex->bind();
        }
        Mesh *m = (Mesh*)t.mesh.get();
        m->bind();
        m->dispatch();
    }

    void render()
    {
        checkGl("pre-frame check");

        if (options.width <= 0 || options.height <= 0)
            return;

        // update framebuffer texture
        if (options.width != widthPrev || options.height != heightPrev
                || options.antialiasingSamples != antialiasingPrev)
        {
            widthPrev = options.width;
            heightPrev = options.height;
            antialiasingPrev = std::max(std::min(options.antialiasingSamples,
                             maxAntialiasingSamples), 1);

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
            if (antialiasingPrev > 1)
            {
                glTexImage2DMultisample(vars.textureTargetType,
                    antialiasingPrev, GL_DEPTH_COMPONENT32,
                    options.width, options.height, GL_TRUE);
            }
            else
            {
                glTexImage2D(vars.textureTargetType, 0, GL_DEPTH_COMPONENT32,
                             options.width, options.height,
                             0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
                glTexParameteri(vars.textureTargetType,
                                GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(vars.textureTargetType,
                                GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            }
            checkGl("update depth texture for rendering");

            // depth texture for sampling
            glActiveTexture(GL_TEXTURE0 + 6);
            if (antialiasingPrev > 1)
            {
                glGenTextures(1, &vars.depthReadTexId);
                glBindTexture(GL_TEXTURE_2D, vars.depthReadTexId);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
                             options.width, options.height,
                             0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
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
            checkGl("update depth texture for sampling");

            // color texture for rendering
            glActiveTexture(GL_TEXTURE0 + 7);
            glGenTextures(1, &vars.colorRenderTexId);
            glBindTexture(vars.textureTargetType, vars.colorRenderTexId);
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
            checkGl("update color texture for rendering");

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
            checkGl("update color texture for sampling");

            // render frame buffer
            glDeleteFramebuffers(1, &vars.frameRenderBufferId);
            glGenFramebuffers(1, &vars.frameRenderBufferId);
            glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                    vars.textureTargetType, vars.depthRenderTexId, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    vars.textureTargetType, vars.colorRenderTexId, 0);
            checkGlFramebuffer();

            // sample frame buffer
            glDeleteFramebuffers(1, &vars.frameReadBufferId);
            glGenFramebuffers(1, &vars.frameReadBufferId);
            glBindFramebuffer(GL_FRAMEBUFFER, vars.frameReadBufferId);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_2D, vars.depthReadTexId, 0);
            checkGlFramebuffer();

            checkGl("update frame buffer");
        }

        // initialize opengl
        glViewport(0, 0, options.width, options.height);
        glActiveTexture(GL_TEXTURE0);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        #ifndef VTSR_OPENGLES
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(0, -1000);
        #endif
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        checkGl("initialized opengl");

        // render opaque
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        for (const DrawTask &t : draws.opaque)
            drawSurface(t);
        checkGl("rendered opaque");

        // render transparent
        glEnable(GL_BLEND);
        for (const DrawTask &t : draws.transparent)
            drawSurface(t);
        checkGl("rendered transparent");

        // render polygon edges
        if (options.renderPolygonEdges)
        {
            glDisable(GL_BLEND);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            for (const DrawTask &it : draws.opaque)
            {
                DrawTask t(it);
                t.flatShading = false;
                t.color[0] = t.color[1] = t.color[2] = t.color[3] = 0;
                drawSurface(t);
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_BLEND);
            checkGl("rendered polygon edges");
        }

        // copy the depth (resolve multisampling)
        if (vars.depthReadTexId != vars.depthRenderTexId)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vars.frameReadBufferId);
            glBlitFramebuffer(0, 0, options.width, options.height,
                              0, 0, options.width, options.height,
                              GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
            checkGl("copied the depth (resolved multisampling)");
        }
        glDisable(GL_DEPTH_TEST);

        // render atmosphere
        if (options.renderAtmosphere
                && body.majorRadius > 0 && body.atmosphereThickness > 0)
        {
            // atmosphere properties
            vec3 camPos = vec4to3(viewProjInv * vec4(0, 0, -1, 1), true);
            mat4f uniInvView = viewProjInv.cast<float>();
            double camRad = length(camPos);
            double lowRad = body.majorRadius;
            double atmRad = body.majorRadius + body.atmosphereThickness;
            double aurDotLow = camRad > lowRad
                    ? -sqrt(sqr(camRad) - sqr(lowRad)) / camRad : 0;
            double aurDotHigh = camRad > atmRad
                    ? -sqrt(sqr(camRad) - sqr(atmRad)) / camRad : 0;
            aurDotHigh = std::max(aurDotHigh, aurDotLow + 1e-4);
            double horizonDistance = camRad > body.majorRadius
                    ? sqrt(sqr(camRad) - sqr(body.majorRadius)) : 0;
            double horizonAngle = camRad > body.majorRadius
                    ? body.majorRadius / camRad : 1;

            // fog properties
            double fogInsideStart = 0;
            double fogInsideFull = sqrt(sqr(atmRad) - sqr(body.majorRadius))
                                    * 0.5;
            double fogOutsideStart = std::max(camRad - body.majorRadius, 0.0);
            double fogOutsideFull = std::max(horizonDistance,
                                             fogOutsideStart + 1);
            double fogFactor = clamp((camRad - body.majorRadius)
                    / body.atmosphereThickness, 0, 1);
            double fogStart = interpolate(fogInsideStart,
                                               fogOutsideStart, fogFactor);
            double fogFull = interpolate(fogInsideFull,
                                              fogOutsideFull, fogFactor);

            // body properties
            vec3f uniCameraPosition = camPos.cast<float>();
            vec3f uniCameraPosNorm = normalize(camPos).cast<float>();
            float uniBody[4]
                = { (float)body.majorRadius, (float)body.minorRadius,
                    (float)body.atmosphereThickness };
            float uniPlanes[4] = { (float)draws.camera.near,
                                   (float)draws.camera.far,
                                   (float)fogStart, (float)fogFull };
            float uniAtmosphere[4] = { (float)aurDotLow, (float)aurDotHigh,
                                       (float)horizonAngle };

            // camera directions
            vec3 near = vec4to3(viewProjInv * vec4(0, 0, -1, 1), true);
            vec3f uniCameraDirections[4] = {
                normalize(vec4to3(viewProjInv * vec4(-1, -1, 1, 1)
                    , true)- near).cast<float>(),
                normalize(vec4to3(viewProjInv * vec4(+1, -1, 1, 1)
                    , true)- near).cast<float>(),
                normalize(vec4to3(viewProjInv * vec4(-1, +1, 1, 1)
                    , true)- near).cast<float>(),
                normalize(vec4to3(viewProjInv * vec4(+1, +1, 1, 1)
                    , true)- near).cast<float>(),
            };

            // shader uniforms
            shaderAtmosphere->bind();
            shaderAtmosphere->uniformVec4(0, body.atmosphereColorLow);
            shaderAtmosphere->uniformVec4(1, body.atmosphereColorHigh);
            shaderAtmosphere->uniformVec4(2, uniBody);
            shaderAtmosphere->uniformVec4(3, uniPlanes);
            shaderAtmosphere->uniformVec4(4, uniAtmosphere);
            shaderAtmosphere->uniformVec3(5, (float*)uniCameraPosition.data());
            shaderAtmosphere->uniformVec3(6, (float*)uniCameraPosNorm.data());
            shaderAtmosphere->uniform(7, (int)draws.camera.mapProjected);
            for (int i = 0; i < 4; i++)
            {
                shaderAtmosphere->uniformVec3(8 + i,
                                (float*)uniCameraDirections[i].data());
            }
            shaderAtmosphere->uniformMat4(12, (float*)uniInvView.data());
            shaderAtmosphere->uniform(13, options.antialiasingSamples);

            // dispatch
            meshQuad->bind();
            meshQuad->dispatch();
            checkGl("rendered atmosphere");
        }

        // render infographics
        for (const DrawTask &t : draws.Infographic)
            drawInfographic(t);
        checkGl("rendered infographics");

        // copy the color (resolve multisampling)
        if (options.colorToTexture
                && vars.colorReadTexId != vars.colorRenderTexId)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vars.frameReadBufferId);
            glBlitFramebuffer(0, 0, options.width, options.height,
                              0, 0, options.width, options.height,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
            checkGl("copied the color (resolved multisampling)");
        }

        // copy the color to screen
        if (options.colorToTargetFrameBuffer)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, options.targetFrameBuffer);
            glBlitFramebuffer(0, 0, options.width, options.height,
                              options.targetViewportX, options.targetViewportY,
                              options.width, options.height,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
            checkGl("copied the color to screen (resolve multisampling)");
        }

        // clear the state
        clearGlState();
    }
};

} // namespace

void initialize()
{
    vts::log(vts::LogLevel::info3, "Initializing vts renderer library");

    // load shader surface
    {
        shaderSurface = std::make_shared<Shader>();
        Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/surface.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/surface.frag.glsl");
        shaderSurface->load(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
        std::vector<uint32> &uls = shaderSurface->uniformLocations;
        GLuint id = shaderSurface->getId();
        uls.push_back(glGetUniformLocation(id, "uniMvp"));
        uls.push_back(glGetUniformLocation(id, "uniMv"));
        uls.push_back(glGetUniformLocation(id, "uniUvMat"));
        uls.push_back(glGetUniformLocation(id, "uniColor"));
        uls.push_back(glGetUniformLocation(id, "uniUvClip"));
        uls.push_back(glGetUniformLocation(id, "uniFlags"));
        glUseProgram(id);
        glUniform1i(glGetUniformLocation(id, "texColor"), 0);
        glUniform1i(glGetUniformLocation(id, "texMask"), 1);
        glUseProgram(0);
    }

    // load shader infographic
    {
        shaderInfographic = std::make_shared<Shader>();
        Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/infographic.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/infographic.frag.glsl");
        shaderInfographic->load(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
        std::vector<uint32> &uls = shaderInfographic->uniformLocations;
        GLuint id = shaderInfographic->getId();
        uls.push_back(glGetUniformLocation(id, "uniMvp"));
        uls.push_back(glGetUniformLocation(id, "uniColor"));
        uls.push_back(glGetUniformLocation(id, "uniUseColorTexture"));
        glUseProgram(id);
        glUniform1i(glGetUniformLocation(id, "texColor"), 0);
        glUniform1i(glGetUniformLocation(id, "texDepth"), 6);
        glUseProgram(0);
    }

    // load shader atmosphere
    {
        shaderAtmosphere = std::make_shared<Shader>();
        Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/atmosphere.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/atmosphere.frag.glsl");
        shaderAtmosphere->load(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
        std::vector<uint32> &uls = shaderAtmosphere->uniformLocations;
        GLuint id = shaderAtmosphere->getId();
        uls.push_back(glGetUniformLocation(id, "uniColorLow"));
        uls.push_back(glGetUniformLocation(id, "uniColorHigh"));
        uls.push_back(glGetUniformLocation(id, "uniBody"));
        uls.push_back(glGetUniformLocation(id, "uniPlanes"));
        uls.push_back(glGetUniformLocation(id, "uniAtmosphere"));
        uls.push_back(glGetUniformLocation(id, "uniCameraPosition"));
        uls.push_back(glGetUniformLocation(id, "uniCameraPosNorm"));
        uls.push_back(glGetUniformLocation(id, "uniProjected"));
        uls.push_back(glGetUniformLocation(id, "uniCameraDirections[0]"));
        uls.push_back(glGetUniformLocation(id, "uniCameraDirections[1]"));
        uls.push_back(glGetUniformLocation(id, "uniCameraDirections[2]"));
        uls.push_back(glGetUniformLocation(id, "uniCameraDirections[3]"));
        uls.push_back(glGetUniformLocation(id, "uniInvView"));
        uls.push_back(glGetUniformLocation(id, "uniMultiSamples"));
        glUseProgram(id);
        glUniform1i(glGetUniformLocation(id, "texDepthSingle"), 6);
        glUniform1i(glGetUniformLocation(id, "texDepthMulti"), 5);
        glUseProgram(0);
    }

    // load shader copy depth
    {
        shaderCopyDepth = std::make_shared<Shader>();
        Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/copyDepth.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/copyDepth.frag.glsl");
        shaderCopyDepth->load(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
        std::vector<uint32> &uls = shaderCopyDepth->uniformLocations;
        GLuint id = shaderCopyDepth->getId();
        uls.push_back(glGetUniformLocation(id, "uniTexPos"));
        glUseProgram(id);
        glUniform1i(glGetUniformLocation(id, "texDepth"), 0);
        glUseProgram(0);
    }

    // load mesh quad
    {
        meshQuad = std::make_shared<Mesh>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer(
                                  "data/meshes/quad.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Triangles);
        spec.attributes.resize(2);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        spec.attributes[1].enable = true;
        spec.attributes[1].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[1].components = 2;
        spec.attributes[1].offset = sizeof(vts::vec3f);
        vts::ResourceInfo info;
        meshQuad->load(info, spec);
    }

    // load mesh rect
    {
        meshRect = std::make_shared<Mesh>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer(
                                  "data/meshes/rect.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Triangles);
        spec.attributes.resize(2);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        spec.attributes[1].enable = true;
        spec.attributes[1].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[1].components = 2;
        spec.attributes[1].offset = sizeof(vts::vec3f);
        vts::ResourceInfo info;
        meshRect->load(info, spec);
    }

    vts::log(vts::LogLevel::info1, "Initialized vts renderer library");
}

void finalize()
{
    vts::log(vts::LogLevel::info3, "Finalizing vts renderer library");

    shaderSurface.reset();
    shaderInfographic.reset();
    shaderAtmosphere.reset();
    meshQuad.reset();

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

    vts::log(vts::LogLevel::info1, "Finalized vts renderer library");
}

RenderOptions::RenderOptions() : width(0), height(0),
    targetFrameBuffer(0), targetViewportX(0), targetViewportY(0),
    antialiasingSamples(1),
    renderAtmosphere(true), renderPolygonEdges(false),
    colorToTargetFrameBuffer(true), colorToTexture(false)
{}

RenderVariables::RenderVariables() :
    frameRenderBufferId(0), frameReadBufferId(0),
    depthRenderTexId(0), depthReadTexId(0),
    colorRenderTexId(0),
    textureTargetType(0)
{}

void loadTexture(ResourceInfo &info, GpuTextureSpec &spec)
{
    auto r = std::make_shared<Texture>();
    r->load(info, spec);
    info.userData = r;
}

void loadMesh(ResourceInfo &info, GpuMeshSpec &spec)
{
    auto r = std::make_shared<Mesh>();
    r->load(info, spec);
    info.userData = r;
}

void render(const RenderOptions &options,
            const MapDraws &draws,
            const MapCelestialBody &body)
{
    Renderer r(options, draws, body);
    r.render();
}

void render(const RenderOptions &options,
            RenderVariables &variables,
            const MapDraws &draws,
            const MapCelestialBody &celestialBody)
{
    render(options, draws, celestialBody);
    variables = vars;
}

void getWorldPosition(const double screenPos[2], double worldPos[3])
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
        checkGlFramebuffer();
        
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
    checkGl("read depth");
    clearGlState();

    if (depth > 1 - 1e-7)
        depth = std::numeric_limits<float>::quiet_NaN();
    depth = depth * 2 - 1;
    x = x / widthPrev * 2 - 1;
    y = y / heightPrev * 2 - 1;
    vec3 res = vec4to3(viewProjInv * vec4(x, y, depth, 1), true);
    for (int i = 0; i < 3; i++)
        worldPos[i] = res[i];
}

} } // namespace vts renderer

