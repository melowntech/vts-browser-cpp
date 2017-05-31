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

#include <vts-browser/map.hpp>
#include <vts-browser/statistics.hpp>
#include <vts-browser/draws.hpp>
#include <vts-browser/buffer.hpp>
#include <vts-browser/resources.hpp>
#include <vts-browser/options.hpp>
#include <vts-browser/exceptions.hpp>
#include <vts-browser/credits.hpp>
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

} // namespace

namespace vts
{
    class MapImpl;
}

MainWindow::MainWindow() :
    camNear(0), camFar(0),
    mousePrevX(0), mousePrevY(0),
    map(nullptr), window(nullptr)
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);
    glfwWindowHint(GLFW_DEPTH_BITS, 32);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

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
    
    { // load shader texture
        shaderTexture = std::make_shared<GpuShaderImpl>();
        vts::Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/texture.vert.glsl");
        vts::Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/texture.frag.glsl");
        shaderTexture->loadShaders(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
        std::vector<vts::uint32> &uls = shaderTexture->uniformLocations;
        GLuint id = shaderTexture->id;
        uls.push_back(glGetUniformLocation(id, "uniMvp"));
        uls.push_back(glGetUniformLocation(id, "uniUvMat"));
        uls.push_back(glGetUniformLocation(id, "uniUvMode"));
        uls.push_back(glGetUniformLocation(id, "uniMaskMode"));
        uls.push_back(glGetUniformLocation(id, "uniTexMode"));
        uls.push_back(glGetUniformLocation(id, "uniAlpha"));
        glUseProgram(id);
        glUniform1i(glGetUniformLocation(id, "texColor"), 0);
        glUniform1i(glGetUniformLocation(id, "texMask"), 1);
    }
    
    { // load shader color
        shaderColor = std::make_shared<GpuShaderImpl>();
        vts::Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/color.vert.glsl");
        vts::Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/color.frag.glsl");
        shaderColor->loadShaders(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
        std::vector<vts::uint32> &uls = shaderColor->uniformLocations;
        GLuint id = shaderColor->id;
        uls.push_back(glGetUniformLocation(id, "uniMvp"));
        uls.push_back(glGetUniformLocation(id, "uniColor"));
    }
    
    { // load mesh mark
        meshMark = std::make_shared<GpuMeshImpl>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer(
                                  "data/meshes/sphere.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Triangles);
        spec.attributes.resize(1);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        vts::ResourceInfo info;
        meshMark->loadMesh(info, spec);
    }
    
    { // load mesh line
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

void MainWindow::mouseButtonCallback(int, int, int)
{
    // do nothing
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
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        y = height - y - 1;
        float depth = std::numeric_limits<float>::quiet_NaN();
        glReadPixels((int)x, (int)y, 1, 1,
                     GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
        depth = depth * 2 - 1;
        x = x / width * 2 - 1;
        y = y / height * 2 - 1;
        Mark mark;
        mark.coord = vts::vec4to3(camViewProj.inverse()
                                  * vts::vec4(x, y, depth, 1), true);
        marks.push_back(mark);
        colorizeMarks();
    }
}

void MainWindow::keyboardUnicodeCallback(unsigned int)
{
    // do nothing
}

void MainWindow::drawVtsTask(vts::DrawTask &t)
{
    if (t.texColor)
    {
        shaderTexture->bind();
        shaderTexture->uniformMat4(0, t.mvp);
        shaderTexture->uniformMat3(1, t.uvm);
        shaderTexture->uniform(2, (int)t.externalUv);
        if (t.texMask)
        {
            shaderTexture->uniform(3, 1);
            glActiveTexture(GL_TEXTURE0 + 1);
            
            ((GpuTextureImpl*)t.texMask.get())->bind();
            glActiveTexture(GL_TEXTURE0 + 0);
        }
        else
            shaderTexture->uniform(3, 0);
        GpuTextureImpl *tex = (GpuTextureImpl*)t.texColor.get();
        tex->bind();
        shaderTexture->uniform(4, (int)tex->grayscale);
        shaderTexture->uniform(5, t.color[3]);
    }
    else
    {
        shaderColor->bind();
        shaderColor->uniformMat4(0, t.mvp);
        shaderColor->uniformVec4(1, t.color);
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
    t.mesh = meshMark;
    memcpy(t.mvp, mvpf.data(), sizeof(t.mvp));
    drawVtsTask(t);
    if (prev)
    {
        t.mesh = meshLine;
        mvp = camViewProj * vts::lookAt(m.coord, prev->coord);
        mvpf = mvp.cast<float>();
        memcpy(t.mvp, mvpf.data(), sizeof(t.mvp));
        drawVtsTask(t);
    }
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
    while (!glfwWindowShouldClose(window))
    {
        checkGl("frame begin");
        double timeFrameStart = glfwGetTime();
        
        try
        {
            map->renderTick(width, height); // calls camera overrides
        }
        catch (const vts::MapConfigException &)
        {
            map->setMapConfigPath("");
            if (mapConfigPaths.size() <= 1)
                throw;
        }
        double timeMapRender = glfwGetTime();

        width = 0;
        height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.2, 0.2, 0.2, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_CULL_FACE);
        //glCullFace(GL_FRONT);
        checkGl("frame preparation");
        
        { // draws
            camViewProj = camProj * camView;
            vts::MapDraws &draws = map->draws();
            for (vts::DrawTask &t : draws.draws)
                drawVtsTask(t);
            shaderColor->bind();
            meshMark->bind();
            Mark *prevMark = nullptr;
            for (Mark &mark : marks)
            {
                drawMark(mark, prevMark);
                prevMark = &mark;
            }
            glBindVertexArray(0);
        }
        checkGl("frame draws");
        
        double timeAppRender = glfwGetTime();
        
        gui.input(); // calls glfwPollEvents()
        gui.render(width, height);
        double timeGui = glfwGetTime();
        
        glfwSwapBuffers(window);
        std::string creditLine = std::string() + "vts-browser-glfw: "
                + map->credits().textFull();
        glfwSetWindowTitle(window, creditLine.c_str());
        double timeFrameFinish = glfwGetTime();
        
        timingMapProcess = timeMapRender - timeFrameStart;
        timingAppProcess = timeAppRender - timeMapRender;
        timingGuiProcess = timeGui - timeAppRender;
        timingTotalFrame = timeFrameFinish - timeFrameStart;
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

