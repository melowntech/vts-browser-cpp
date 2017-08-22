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

#include <limits>
#include <cmath>
#include <cstdio>
#include <unistd.h>

#include <vts-browser/map.hpp>
#include <vts-browser/statistics.hpp>
#include <vts-browser/draws.hpp>
#include <vts-browser/buffer.hpp>
#include <vts-browser/resources.hpp>
#include <vts-browser/options.hpp>
#include <vts-browser/exceptions.hpp>
#include <vts-browser/credits.hpp>
#include <vts-browser/log.hpp>
#include <vts-browser/celestial.hpp>
#include "mainWindow.hpp"
#include <GLFW/glfw3.h>

namespace
{

using vts::readInternalMemoryBuffer;

void mousePositionCallback(GLFWwindow *window, double xpos, double ypos)
{
    MainWindow *m = (MainWindow*)glfwGetWindowUserPointer(window);
    m->gui.mousePositionCallback(xpos, ypos);
}

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    MainWindow *m = (MainWindow*)glfwGetWindowUserPointer(window);
    m->gui.mouseButtonCallback(button, action, mods);
}

void mouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    MainWindow *m = (MainWindow*)glfwGetWindowUserPointer(window);
    m->gui.mouseScrollCallback(xoffset, yoffset);
}

void keyboardCallback(GLFWwindow *window, int key, int scancode,
                                            int action, int mods)
{
    MainWindow *m = (MainWindow*)glfwGetWindowUserPointer(window);
    m->gui.keyboardCallback(key, scancode, action, mods);
}

void keyboardUnicodeCallback(GLFWwindow *window, unsigned int codepoint)
{
    MainWindow *m = (MainWindow*)glfwGetWindowUserPointer(window);
    m->gui.keyboardUnicodeCallback(codepoint);
}

double sqr(double a)
{
    return a * a;
}

} // namespace

AppOptions::AppOptions() :
    antialiasing(1),
    screenshotOnFullRender(false),
    closeOnFullRender(false),
    purgeDiskCache(false),
    renderAtmosphere(true),
    renderPolygonEdges(false)
{}

MainWindow::MainWindow(vts::Map *map, const AppOptions &appOptions) :
    appOptions(appOptions),
    camNear(0), camFar(0),
    mousePrevX(0), mousePrevY(0),
    dblClickInitTime(0), dblClickState(0),
    width(0), height(0), widthPrev(0), heightPrev(0), antialiasingPrev(0),
    frameRenderBufferId(0), frameSampleBufferId(0),
    depthRenderTexId(0), depthSampleTexId(0), colorTexId(0),
    map(map), window(nullptr)
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);
    glfwWindowHint(GLFW_DEPTH_BITS, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

    glfwWindowHint(GLFW_VISIBLE, true);
    window = glfwCreateWindow(800, 600, "renderer-glfw", NULL, NULL);
    if (!window)
        throw std::runtime_error("Failed to create window "
                                 "(unsupported OpenGL version?)");
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    gladLoadGLLoader((GLADloadproc)&glfwGetProcAddress);
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, &::mousePositionCallback);
    glfwSetMouseButtonCallback(window, &::mouseButtonCallback);
    glfwSetScrollCallback(window, &::mouseScrollCallback);
    glfwSetKeyCallback(window, &::keyboardCallback);
    glfwSetCharCallback(window, &::keyboardUnicodeCallback);

    // check for extensions
    anisotropicFilteringAvailable
            = glfwExtensionSupported("GL_EXT_texture_filter_anisotropic");
    openglDebugAvailable
            = glfwExtensionSupported("GL_KHR_debug");

    initializeGpuContext();

    // load shader surface
    {
        shaderSurface = std::make_shared<GpuShaderImpl>();
        vts::Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/surface.vert.glsl");
        vts::Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/surface.frag.glsl");
        shaderSurface->loadShaders(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
        std::vector<vts::uint32> &uls = shaderSurface->uniformLocations;
        GLuint id = shaderSurface->id;
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
        shaderInfographic = std::make_shared<GpuShaderImpl>();
        vts::Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/infographic.vert.glsl");
        vts::Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/infographic.frag.glsl");
        shaderInfographic->loadShaders(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
        std::vector<vts::uint32> &uls = shaderInfographic->uniformLocations;
        GLuint id = shaderInfographic->id;
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
        shaderAtmosphere = std::make_shared<GpuShaderImpl>();
        vts::Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/atmosphere.vert.glsl");
        vts::Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/atmosphere.frag.glsl");
        shaderAtmosphere->loadShaders(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
        std::vector<vts::uint32> &uls = shaderAtmosphere->uniformLocations;
        GLuint id = shaderAtmosphere->id;
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

    // load mesh mark
    {
        meshSphere = std::make_shared<GpuMeshImpl>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer(
                                  "data/meshes/sphere.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Triangles);
        spec.attributes.resize(1);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        vts::ResourceInfo info;
        meshSphere->loadMesh(info, spec);
    }

    // load mesh line
    {
        meshLine = std::make_shared<GpuMeshImpl>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer(
                                  "data/meshes/line.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Lines);
        spec.attributes.resize(1);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        vts::ResourceInfo info;
        meshLine->loadMesh(info, spec);
    }

    // load mesh quad
    {
        meshQuad = std::make_shared<GpuMeshImpl>();
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
        meshQuad->loadMesh(info, spec);
    }
}

MainWindow::~MainWindow()
{
    if (map)
        map->renderFinalize();
    glfwDestroyWindow(window);
    window = nullptr;
}

void MainWindow::mousePositionCallback(double xpos, double ypos)
{
    double p[3] = { xpos - mousePrevX, ypos - mousePrevY, 0 };
    int mode = 0;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
            mode = 2;
        else
            mode = 1;
    }
    else
    {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS
        || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
            mode = 2;
    }
    switch (mode)
    {
    case 1:
        map->pan(p);
        break;
    case 2:
        map->rotate(p);
        break;
    }
    mousePrevX = xpos;
    mousePrevY = ypos;
}

void MainWindow::mouseButtonCallback(int button, int action, int mods)
{
    static const double dblClickThreshold = 0.22;
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        double n = glfwGetTime();
        switch (action)
        {
        case GLFW_PRESS:
            if (dblClickState == 2 && dblClickInitTime + dblClickThreshold > n)
            {
                mouseDblClickCallback(mods);
                dblClickState = 0;
            }
            else
            {
                dblClickInitTime = n;
                dblClickState = 1;
            }
            break;
        case GLFW_RELEASE:
            if (dblClickState == 1 && dblClickInitTime + dblClickThreshold > n)
                dblClickState = 2;
            else
                dblClickState = 0;
            break;
        }
    }
    else
    {
        dblClickInitTime = 0;
        dblClickState = 0;
    }
}

void MainWindow::mouseDblClickCallback(int)
{
    vts::vec3 posPhys = getWorldPositionFromCursor();
    if (posPhys(0) != posPhys(0))
        return;
    double posNav[3];
    map->convert(posPhys.data(), posNav,
                 vts::Srs::Physical, vts::Srs::Navigation);
    map->setPositionPoint(posNav, vts::NavigationType::Quick);
}

void MainWindow::mouseScrollCallback(double, double yoffset)
{
    map->zoom(yoffset * 120);
}

void MainWindow::keyboardCallback(int key, int, int action, int)
{
    // marks
    if (action == GLFW_RELEASE && key == GLFW_KEY_M)
    {
        Mark mark;
        mark.coord = getWorldPositionFromCursor();
        if (mark.coord(0) != mark.coord(0))
            return;
        marks.push_back(mark);
        colorizeMarks();
    }
    // north-up button
    if (action == GLFW_RELEASE && key == GLFW_KEY_SPACE)
    {
        map->setPositionRotation({0,270,0}, vts::NavigationType::Quick);
        map->resetNavigationMode();
    }
}

void MainWindow::keyboardUnicodeCallback(unsigned int)
{
    // do nothing
}

void MainWindow::drawVtsTaskSurface(const vts::DrawTask &t)
{
    GpuTextureImpl *tex = (GpuTextureImpl*)t.texColor.get();
    GpuMeshImpl *m = (GpuMeshImpl*)t.mesh.get();
    shaderSurface->bind();
    shaderSurface->uniformMat4(0, t.mvp);
    shaderSurface->uniformMat3(2, t.uvm);
    shaderSurface->uniformVec4(3, t.color);
    shaderSurface->uniformVec4(4, t.uvClip);
    float flags[4] = {
        t.texMask ? 1.f : -1.f,
        tex->grayscale ? 1.f : -1.f,
        t.flatShading ? 1.f : -1.f,
        t.externalUv ? 1.f : -1.f
    };
    shaderSurface->uniformVec4(5, flags);
    if (t.flatShading)
    {
        vts::mat4f mv = vts::mat4f(t.mvp);
        mv = camProj.cast<float>().inverse() * mv;
        shaderSurface->uniformMat4(1, (float*)mv.data());
    }
    if (t.texMask)
    {
        glActiveTexture(GL_TEXTURE0 + 1);
        ((GpuTextureImpl*)t.texMask.get())->bind();
        glActiveTexture(GL_TEXTURE0 + 0);
    }
    tex->bind();
    m->bind();
    m->dispatch();
}

void MainWindow::drawVtsTaskInfographic(const vts::DrawTask &t)
{
    shaderInfographic->bind();
    shaderInfographic->uniformMat4(0, t.mvp);
    shaderInfographic->uniformVec4(1, t.color);
    shaderInfographic->uniform(2, (int)(!!t.texColor));
    if (t.texColor)
    {
        GpuTextureImpl *tex = (GpuTextureImpl*)t.texColor.get();
        tex->bind();
    }
    GpuMeshImpl *m = (GpuMeshImpl*)t.mesh.get();
    m->bind();
    m->dispatch();
}

void MainWindow::drawMark(const Mark &m, const Mark *prev)
{
    vts::mat4 mvp = camViewProj
            * vts::translationMatrix(m.coord)
            * vts::scaleMatrix(map->getPositionViewExtent() * 0.005);
    vts::mat4f mvpf = mvp.cast<float>();
    vts::DrawTask t;
    vts::vec4f c = vts::vec3to4f(m.color, 1);
    for (int i = 0; i < 4; i++)
        t.color[i] = c(i);
    t.mesh = meshSphere;
    memcpy(t.mvp, mvpf.data(), sizeof(t.mvp));
    drawVtsTaskInfographic(t);
    if (prev)
    {
        t.mesh = meshLine;
        mvp = camViewProj * vts::lookAt(m.coord, prev->coord);
        mvpf = mvp.cast<float>();
        memcpy(t.mvp, mvpf.data(), sizeof(t.mvp));
        drawVtsTaskInfographic(t);
    }
}

void MainWindow::renderFrame()
{
    const vts::MapDraws &draws = map->draws();

    checkGl("pre-frame check");

    // update framebuffer texture
    if (width != widthPrev || height != heightPrev
            || appOptions.antialiasing != antialiasingPrev)
    {
        widthPrev = width;
        heightPrev = height;
        antialiasingPrev = appOptions.antialiasing;

        GLenum target = antialiasingPrev > 1 ? GL_TEXTURE_2D_MULTISAMPLE
                                             : GL_TEXTURE_2D;

        // delete old textures
        glDeleteTextures(1, &depthSampleTexId);
        if (depthRenderTexId != depthSampleTexId)
            glDeleteTextures(1, &depthRenderTexId);
        glDeleteTextures(1, &colorTexId);
        depthSampleTexId = depthRenderTexId = colorTexId = 0;

        // depth texture for rendering
        glActiveTexture(GL_TEXTURE0 + 5);
        glGenTextures(1, &depthRenderTexId);
        glBindTexture(target, depthRenderTexId);
        if (antialiasingPrev > 1)
            glTexImage2DMultisample(target, antialiasingPrev,
                                GL_DEPTH_COMPONENT32, width, height, GL_TRUE);
        else
        {
            glTexImage2D(target, 0, GL_DEPTH_COMPONENT32, width, height,
                         0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
        checkGl("update depth texture");

        // depth texture for sampling
        glActiveTexture(GL_TEXTURE0 + 6);
        if (antialiasingPrev > 1)
        {
            glGenTextures(1, &depthSampleTexId);
            glBindTexture(GL_TEXTURE_2D, depthSampleTexId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height,
                         0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
        else
        {
            depthSampleTexId = depthRenderTexId;
            glBindTexture(GL_TEXTURE_2D, depthSampleTexId);
        }

        // color texture
        glActiveTexture(GL_TEXTURE0 + 7);
        glGenTextures(1, &colorTexId);
        glBindTexture(target, colorTexId);
        if (antialiasingPrev > 1)
            glTexImage2DMultisample(target, antialiasingPrev,
                                GL_RGB8, width, height, GL_TRUE);
        else
        {
            glTexImage2D(target, 0, GL_RGB8, width, height,
                         0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
        checkGl("update color texture");

        // render frame buffer
        glDeleteFramebuffers(1, &frameRenderBufferId);
        glGenFramebuffers(1, &frameRenderBufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, frameRenderBufferId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               target, depthRenderTexId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               target, colorTexId, 0);
        checkGlFramebuffer();

        // sample frame buffer
        glDeleteFramebuffers(1, &frameSampleBufferId);
        glGenFramebuffers(1, &frameSampleBufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, frameSampleBufferId);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                             depthSampleTexId, 0);
        checkGlFramebuffer();

        checkGl("update frame buffer");
    }

    // initialize opengl
    glViewport(0, 0, width, height);
    glActiveTexture(GL_TEXTURE0);
    glBindFramebuffer(GL_FRAMEBUFFER, frameRenderBufferId);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(0, -1000);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render opaque
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    for (const vts::DrawTask &t : draws.opaque)
        drawVtsTaskSurface(t);

    // render transparent
    glEnable(GL_BLEND);
    for (const vts::DrawTask &t : draws.transparent)
        drawVtsTaskSurface(t);

    // render polygon edges
    if (appOptions.renderPolygonEdges)
    {
        glDisable(GL_BLEND);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        for (const vts::DrawTask &it : draws.opaque)
        {
            vts::DrawTask t(it);
            t.flatShading = false;
            t.color[0] = t.color[1] = t.color[2] = t.color[3] = 0;
            drawVtsTaskSurface(t);
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_BLEND);
    }

    // copy the depth (resolve multisampling)
    if (depthSampleTexId != depthRenderTexId)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, frameRenderBufferId);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameSampleBufferId);
        glBlitFramebuffer(0, 0, width, height,
                          0, 0, width, height,
                          GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, frameRenderBufferId);
    }
    glDisable(GL_DEPTH_TEST);

    // render atmosphere
    const vts::MapCelestialBody &body = map->celestialBody();
    if (appOptions.renderAtmosphere
            && body.majorRadius > 0 && body.atmosphereThickness > 0)
    {
        // atmosphere properties
        vts::mat4 inv = camViewProj.inverse();
        vts::vec3 camPos = vts::vec4to3(inv * vts::vec4(0, 0, -1, 1), true);
        vts::mat4f uniInvView = inv.cast<float>();
        double camRad = vts::length(camPos);
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
        double fogInsideFull = sqrt(sqr(atmRad) - sqr(body.majorRadius)) * 0.5;
        double fogOutsideStart = std::max(camRad - body.majorRadius, 0.0);
        double fogOutsideFull = std::max(horizonDistance, fogOutsideStart + 1);
        double fogFactor = vts::clamp((camRad - body.majorRadius)
                / body.atmosphereThickness, 0, 1);
        double fogStart = vts::interpolate(fogInsideStart,
                                           fogOutsideStart, fogFactor);
        double fogFull = vts::interpolate(fogInsideFull,
                                          fogOutsideFull, fogFactor);

        // body properties
        vts::vec3f uniCameraPosition = camPos.cast<float>();
        vts::vec3f uniCameraPosNorm = vts::normalize(camPos).cast<float>();
        float uniBody[4]
            = { (float)body.majorRadius, (float)body.minorRadius,
                (float)body.atmosphereThickness };
        float uniPlanes[4] = { (float)camNear, (float)camFar,
                               (float)fogStart, (float)fogFull };
        float uniAtmosphere[4] = { (float)aurDotLow, (float)aurDotHigh,
                                   (float)horizonAngle };

        // camera directions
        vts::vec3 near = vts::vec4to3(inv * vts::vec4(0, 0, -1, 1), true);
        vts::vec3f uniCameraDirections[4] = {
            vts::normalize(vts::vec4to3(inv * vts::vec4(-1, -1, 1, 1)
                , true)- near).cast<float>(),
            vts::normalize(vts::vec4to3(inv * vts::vec4(+1, -1, 1, 1)
                , true)- near).cast<float>(),
            vts::normalize(vts::vec4to3(inv * vts::vec4(-1, +1, 1, 1)
                , true)- near).cast<float>(),
            vts::normalize(vts::vec4to3(inv * vts::vec4(+1, +1, 1, 1)
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
        shaderAtmosphere->uniform(7, (int)map->getMapProjected());
        for (int i = 0; i < 4; i++)
        {
            shaderAtmosphere->uniformVec3(8 + i,
                            (float*)uniCameraDirections[i].data());
        }
        shaderAtmosphere->uniformMat4(12, (float*)uniInvView.data());
        shaderAtmosphere->uniform(13, (int)appOptions.antialiasing);

        // dispatch
        meshQuad->bind();
        meshQuad->dispatch();
    }

    // render infographics
    for (const vts::DrawTask &t : draws.Infographic)
        drawVtsTaskInfographic(t);

    // render marks
    {
        Mark *prevMark = nullptr;
        for (Mark &mark : marks)
        {
            drawMark(mark, prevMark);
            prevMark = &mark;
        }
    }

    // render gui
    gui.render(width, height);

    // copy the color to screen
    glBindFramebuffer(GL_READ_FRAMEBUFFER, frameRenderBufferId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, width, height,
                      0, 0, width, height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindVertexArray(0);
    checkGl("frame finished");
}

void MainWindow::loadTexture(vts::ResourceInfo &info,
                             const vts::GpuTextureSpec &spec)
{
    auto r = std::make_shared<GpuTextureImpl>();
    r->loadTexture(info, spec);
    info.userData = r;
}

void MainWindow::loadMesh(vts::ResourceInfo &info,
                          const vts::GpuMeshSpec &spec)
{
    auto r = std::make_shared<GpuMeshImpl>();
    r->loadMesh(info, spec);
    info.userData = r;
}

void MainWindow::run()
{
    if (appOptions.purgeDiskCache)
        map->purgeDiskCache();

    glfwGetFramebufferSize(window, &width, &height);
    map->setWindowSize(width, height);

    // this application uses separate thread for resource processing,
    //   therefore it is safe to process as many resources as possible
    //   in single dataTick without causing any lag spikes
    map->options().maxResourceProcessesPerTick = -1;

    setMapConfigPath(appOptions.paths[0]);
    map->callbacks().loadTexture = std::bind(&MainWindow::loadTexture, this,
                std::placeholders::_1, std::placeholders::_2);
    map->callbacks().loadMesh = std::bind(&MainWindow::loadMesh, this,
                std::placeholders::_1, std::placeholders::_2);
    map->callbacks().cameraOverrideView
            = std::bind(&MainWindow::cameraOverrideView,
                                this, std::placeholders::_1);
    map->callbacks().cameraOverrideProj
            = std::bind(&MainWindow::cameraOverrideProj,
                                this, std::placeholders::_1);
    map->callbacks().cameraOverrideFovAspectNearFar = std::bind(
                &MainWindow::cameraOverrideParam, this,
                std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4);
    map->renderInitialize();
    gui.initialize(this);

    bool initialPositionSet = false;

    while (!glfwWindowShouldClose(window))
    {
        if (!initialPositionSet && map->getMapConfigReady())
        {
            initialPositionSet = true;
            if (!appOptions.initialPosition.empty())
            {
                try
                {
                    map->setPositionUrl(appOptions.initialPosition,
                                        vts::NavigationType::Instant);
                }
                catch (...)
                {
                    vts::log(vts::LogLevel::warn3,
                             "failed to set initial position");
                }
            }
        }

        gui.input(); // calls glfwPollEvents()

        checkGl("frame begin");
        double timeFrameStart = glfwGetTime();

        try
        {
            glfwGetFramebufferSize(window, &width, &height);
            map->setWindowSize(width, height);
            map->renderTickPrepare();
            map->renderTickRender(); // calls camera overrides
            camViewProj = camProj * camView;
        }
        catch (const vts::MapConfigException &e)
        {
            std::stringstream s;
            s << "Exception <" << e.what() << ">";
            vts::log(vts::LogLevel::err4, s.str());
            if (appOptions.paths.size() > 1)
                setMapConfigPath(MapPaths());
            else
                throw;
        }
        double timeMapRender = glfwGetTime();

        renderFrame();

        double timeAppRender = glfwGetTime();

        if (map->statistics().renderTicks % 120 == 0)
        {
            std::string creditLine = std::string() + "vts-browser-glfw: "
                    + map->credits().textFull();
            glfwSetWindowTitle(window, creditLine.c_str());
        }

        glfwSwapBuffers(window);
        double timeFrameFinish = glfwGetTime();

        // temporary workaround for when v-sync is missing
        long duration = (timeFrameFinish - timeFrameStart) * 1000000;
        if (duration < 16000)
        {
            usleep(16000 - duration);
            timeFrameFinish = glfwGetTime();
        }

        timingMapProcess = timeMapRender - timeFrameStart;
        timingAppProcess = timeAppRender - timeMapRender;
        timingTotalFrame = timeFrameFinish - timeFrameStart;

        if (appOptions.closeOnFullRender && map->getMapRenderComplete())
            glfwSetWindowShouldClose(window, true);
    }
    gui.finalize();
}

void MainWindow::colorizeMarks()
{
    if (marks.empty())
        return;
    float mul = 1.0f / marks.size();
    int index = 0;
    for (Mark &m : marks)
        m.color = vts::convertHsvToRgb(vts::vec3f(index++ * mul, 1, 1));
}

vts::vec3 MainWindow::getWorldPositionFromCursor()
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    y = height - y - 1;
    float depth = std::numeric_limits<float>::quiet_NaN();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, frameSampleBufferId);
    glReadPixels((int)x, (int)y, 1, 1,
                 GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    if (depth > 1 - 1e-7)
        depth = std::numeric_limits<float>::quiet_NaN();
    depth = depth * 2 - 1;
    x = x / width * 2 - 1;
    y = y / height * 2 - 1;
    return vts::vec4to3(camViewProj.inverse()
                              * vts::vec4(x, y, depth, 1), true);
}

void MainWindow::cameraOverrideParam(double &, double &,
                                     double &near, double &far)
{
    camNear = near;
    camFar = far;
}

void MainWindow::cameraOverrideView(double *mat)
{
    for (int i = 0; i < 16; i++)
        camView(i) = mat[i];
}

void MainWindow::cameraOverrideProj(double *mat)
{
    for (int i = 0; i < 16; i++)
        camProj(i) = mat[i];
}

void MainWindow::setMapConfigPath(const MapPaths &paths)
{
    map->setMapConfigPath(paths.mapConfig, paths.auth, paths.sri);
}

