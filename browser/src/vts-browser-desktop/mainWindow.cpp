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

#include <thread>
#include <chrono>
#include <limits>
#include <cmath>
#include <cstdio>

#include <vts-browser/buffer.hpp>
#include <vts-browser/exceptions.hpp>
#include <vts-browser/celestial.hpp>
#include <vts-browser/mapCallbacks.hpp>
#include <vts-browser/mapStatistics.hpp>
#include <vts-browser/cameraDraws.hpp>
#include <vts-browser/cameraCredits.hpp>
#include <vts-browser/position.hpp>

#include <GLFW/glfw3.h>
#include <optick.h>

#include "mainWindow.hpp"

void initializeDesktopData();

namespace
{

struct Initializer
{
    Initializer()
    {
        initializeDesktopData();
    }
} initializer;

void windowSwap(GLFWwindow *window)
{
    OPTICK_EVENT();
    glfwSwapBuffers(window);
}

} // namespace

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    MainWindow *w = (MainWindow *)glfwGetWindowUserPointer(window);
    w->key_callback(key, scancode, action, mods);
}

void character_callback(GLFWwindow *window, unsigned int codepoint)
{
    MainWindow *w = (MainWindow *)glfwGetWindowUserPointer(window);
    w->character_callback(codepoint);
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
    MainWindow *w = (MainWindow *)glfwGetWindowUserPointer(window);
    w->cursor_position_callback(xpos, ypos);
    w->lastXPos = xpos;
    w->lastYPos = ypos;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    MainWindow *w = (MainWindow *)glfwGetWindowUserPointer(window);
    w->mouse_button_callback(button, action, mods);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    MainWindow *w = (MainWindow *)glfwGetWindowUserPointer(window);
    w->scroll_callback(xoffset, yoffset);
}

MainWindow::MainWindow(GLFWwindow *window, vts::Map *map, vts::Camera *camera, vts::Navigation *navigation, const AppOptions &appOptions, const vts::renderer::RenderOptions &renderOptions) : appOptions(appOptions), map(map), camera(camera), navigation(navigation), window(window)
{
    context.bindLoadFunctions(map);
    view = context.createView(camera);
    view->options() = renderOptions;

    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, &::key_callback);
    glfwSetCharCallback(window, &::character_callback);
    glfwSetCursorPosCallback(window, &::cursor_position_callback);
    glfwSetMouseButtonCallback(window, &::mouse_button_callback);
    glfwSetScrollCallback(window, &::scroll_callback);
}

MainWindow::~MainWindow()
{}

void MainWindow::loadResources()
{
    // load mesh sphere
    {
        meshSphere = std::make_shared<vts::renderer::Mesh>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer("data/meshes/sphere.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Triangles);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        spec.attributes[1].enable = true;
        spec.attributes[1].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[1].components = 2;
        spec.attributes[1].offset = sizeof(vts::vec3f);
        vts::ResourceInfo info;
        meshSphere->load(info, spec, "data/meshes/sphere.obj");
    }

    // load mesh line
    {
        meshLine = std::make_shared<vts::renderer::Mesh>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer("data/meshes/line.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Lines);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        spec.attributes[1].enable = true;
        spec.attributes[1].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[1].components = 2;
        spec.attributes[1].offset = sizeof(vts::vec3f);
        vts::ResourceInfo info;
        meshLine->load(info, spec, "data/meshes/line.obj");
    }
}

void MainWindow::renderFrame()
{
    OPTICK_EVENT();

    vts::renderer::RenderOptions &ro = view->options();
    view->render();

    // compas
    if (appOptions.renderCompas)
    {
        OPTICK_EVENT("compas");
        double size = std::min(ro.width, ro.height) * 0.09;
        double offset = size * (0.5 + 0.2);
        double posSize[3] = { offset, offset, size };
        double rot[3];
        navigation->getRotation(rot);
        view->renderCompass(posSize, rot);
    }

    // gui
    {
        OPTICK_EVENT("gui");
        gui.render(ro.targetViewportW, ro.targetViewportH);
    }
}

void MainWindow::prepareMarks()
{
    vts::mat4 view = vts::rawToMat4(camera->draws().camera.view);

    const Mark *prev = nullptr;
    for (const Mark &m : marks)
    {
        vts::mat4 mv = view
                * vts::translationMatrix(m.coord)
                * vts::scaleMatrix(navigation->getViewExtent() * 0.005);
        vts::mat4f mvf = mv.cast<float>();
        vts::DrawInfographicsTask t;
        vts::vec4f c = vts::vec3to4(m.color, 1);
        for (int i = 0; i < 4; i++)
            t.color[i] = c(i);
        t.mesh = meshSphere;
        vts::matToRaw(mvf, t.mv);
        camera->draws().infographics.push_back(t);
        if (prev)
        {
            t.mesh = meshLine;
            mv = view * vts::lookAt(m.coord, prev->coord);
            mvf = mv.cast<float>();
            vts::matToRaw(mvf, t.mv);
            camera->draws().infographics.push_back(t);
        }
        prev = &m;
    }
}

void MainWindow::key_callback(int key, int scancode, int action, int mods)
{
    if (gui.key_callback(key, scancode, action, mods))
        return;

    if (action != GLFW_RELEASE)
        return;

    switch (key)
    {
    // fullscreen toggle
    case GLFW_KEY_F11:
    {
        // todo
    } break;

    // screenshot
    case GLFW_KEY_P:
    {
        makeScreenshot();
    } break;

    // gui toggle
    case GLFW_KEY_G:
    {
        const bool control = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        if (control)
            appOptions.guiVisible = !appOptions.guiVisible;
    } break;

    // add mark
    case GLFW_KEY_M:
    {
        Mark mark;
        mark.coord = getWorldPositionFromCursor();
        if (mark.coord(0) == mark.coord(0))
        {
            marks.push_back(mark);
            colorizeMarks();
        }
    } break;

    // north-up button
    case GLFW_KEY_SPACE:
    {
        if (map->getMapconfigAvailable())
        {
            navigation->setRotation({ 0,270,0 });
            navigation->options().type = vts::NavigationType::Quick;
            navigation->resetNavigationMode();
        }
    } break;
    }
}

void MainWindow::character_callback(unsigned int codepoint)
{
    gui.character_callback(codepoint);
}

void MainWindow::cursor_position_callback(double xpos, double ypos)
{
    if (gui.cursor_position_callback(xpos, ypos))
        return;

    const bool leftButt = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool midButt = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    const bool rightButt = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const uint32 buttons = (leftButt ? 1 : 0) + (midButt ? 2 : 0) + (rightButt ? 4 : 0);
    const bool control = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    const bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

    // camera panning or rotating
    int mode = 0;
    if (buttons == 1)
    {
        if (control || shift)
            mode = 2;
        else
            mode = 1;
    }
    else if (buttons == 2 || buttons == 4)
        mode = 2;
    const double p[3] = { (xpos - lastXPos) / contentScale, (ypos - lastYPos) / contentScale, 0 };
    switch (mode)
    {
    case 1:
        navigation->pan(p);
        navigation->options().type = vts::NavigationType::Quick;
        break;
    case 2:
        navigation->rotate(p);
        navigation->options().type = vts::NavigationType::Quick;
        break;
    }
}

void MainWindow::mouse_button_callback(int button, int action, int mods)
{
    if (gui.mouse_button_callback(button, action, mods))
        return;

    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return;

    // detect double click (quick and dirty)
    static double lastPressTime = glfwGetTime();
    const double pressTime = glfwGetTime();
    const double diff = pressTime - lastPressTime;
    lastPressTime = pressTime;
    if (diff > 0.001 && diff < 0.5)
    {
        // jump to cursor position
        vts::vec3 posPhys = getWorldPositionFromCursor();
        if (!std::isnan(posPhys(0)))
        {
            double posNav[3];
            map->convert(posPhys.data(), posNav, vts::Srs::Physical, vts::Srs::Navigation);
            navigation->setPoint(posNav);
            navigation->options().type = vts::NavigationType::Quick;
        }
    }
}

void MainWindow::scroll_callback(double xoffset, double yoffset)
{
    if (gui.scroll_callback(xoffset, yoffset))
        return;

    const bool control = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    const bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    if (control && shift)
    {
        // gui zoom
        appOptions.guiScale *= std::pow(1.05, yoffset);
    }
    else
    {
        // map zoom
        navigation->zoom(yoffset);
        navigation->options().type = vts::NavigationType::Quick;
    }
}

void MainWindow::processEvents()
{
    OPTICK_EVENT();
    gui.inputBegin();
    glfwPollEvents();
    gui.inputEnd();
}

void MainWindow::updateWindowSize()
{
    // update resolution
    vts::renderer::RenderOptions &ro = view->options();
    static_assert(sizeof(int) == sizeof(ro.width), "invalid reinterpret cast");
    glfwGetFramebufferSize(window, (int*)&ro.width, (int*)&ro.height);
    ro.targetViewportW = ro.width;
    ro.targetViewportH = ro.height;
    ro.width *= appOptions.oversampleRender;
    ro.height *= appOptions.oversampleRender;
    camera->setViewportSize(ro.width, ro.height);

    // update scaling
    float xscale, yscale;
#if GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR > 3003
    glfwGetWindowContentScale(window, &xscale, &yscale);
#else
    xscale = yscale = 1;
#endif
    contentScale = std::max(xscale, yscale);
    gui.scale(appOptions.guiScale * contentScale);
    map->options().pixelsPerInch = 96 * contentScale;
    gui.visible(appOptions.guiVisible);
}

void MainWindow::run()
{
    if (appOptions.purgeDiskCache)
        map->purgeDiskCache();
    loadResources();
    setMapConfigPath(appOptions.paths[0]);
    gui.initialize(this);
    glfwShowWindow(window);
    updateWindowSize();

    if (!appOptions.initialPosition.empty())
    {
        map->callbacks().mapconfigAvailable = [&](){
            vts::log(vts::LogLevel::info2, "Setting initial position");
            try
            {
                navigation->setPosition(vts::Position(appOptions.initialPosition));
                navigation->options().type = vts::NavigationType::Instant;
            }
            catch (...)
            {
                vts::log(vts::LogLevel::warn3, "Failed to set initial position");
            }
            map->callbacks().mapconfigAvailable = {};
        };
    }

    auto lastTime = std::chrono::high_resolution_clock::now();
    double accumulatedTime = 0;
    while (!glfwWindowShouldClose(window))
    {
        OPTICK_FRAME("frame");
        auto time1 = std::chrono::high_resolution_clock::now();
        try
        {
            updateWindowSize();
            map->renderUpdate(timingTotalFrame);
            camera->renderUpdate();
        }
        catch (const vts::MapconfigException &e)
        {
            std::stringstream s;
            s << "Exception <" << e.what() << ">";
            vts::log(vts::LogLevel::err4, s.str());
            if (appOptions.paths.size() > 1)
                setMapConfigPath(MapPaths());
            else
                throw;
        }

        auto time2 = std::chrono::high_resolution_clock::now();
        processEvents();
        prepareMarks();
        renderFrame();
        bool renderCompleted = map->getMapRenderComplete();
        if (appOptions.screenshotOnFullRender && renderCompleted)
        {
            appOptions.screenshotOnFullRender = false;
            makeScreenshot();
        }
        if (appOptions.closeOnFullRender && renderCompleted)
            glfwSetWindowShouldClose(window, true);
        if (map->statistics().renderTicks % 120 == 0)
        {
            std::string creditLine = std::string() + "vts-browser-desktop: " + camera->credits().textFull();
            glfwSetWindowTitle(window, creditLine.c_str());
        }

        auto time3 = std::chrono::high_resolution_clock::now();
        windowSwap(window);

        // simulated fps slowdown
        switch (appOptions.simulatedFpsSlowdown)
        {
        case 1:
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            break;
        case 2:
            std::this_thread::sleep_for(std::chrono::milliseconds(std::sin(accumulatedTime * 0.1) < 0 ? 50 : 0));
            break;
        }

        auto time4 = std::chrono::high_resolution_clock::now();
        timingMapProcess = std::chrono::duration<double>(time2 - time1).count();
        timingAppProcess = std::chrono::duration<double>(time3 - time2).count();
        timingTotalFrame = std::chrono::duration<double>(time4 - lastTime).count();
        lastTime = time4;
        accumulatedTime += timingTotalFrame;

        timingMapSmooth.add(timingMapProcess);
        timingFrameSmooth.add(timingTotalFrame);
    }

    // closing the whole app may take some time (waiting on pending downloads)
    //   therefore we hide the window here so that the user
    //   does not get disturbed by it
    glfwHideWindow(window);

    gui.finalize();
    map->renderFinalize();
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
    if (!map->getMapconfigAvailable())
        return vts::nan3();
    double screenPos[2] = {};
    glfwGetCursorPos(window, &screenPos[0], &screenPos[1]);
    vts::vec3 result;
    view->getWorldPosition(screenPos, result.data());
    return result;
}

void MainWindow::setMapConfigPath(const MapPaths &paths)
{
    map->setMapconfigPath(paths.mapConfig, paths.auth);
}

