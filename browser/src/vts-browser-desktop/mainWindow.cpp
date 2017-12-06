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
#include <vts-renderer/classes.hpp>

#include <SDL2/SDL.h>

#include "mainWindow.hpp"

AppOptions::AppOptions() :
    screenshotOnFullRender(false),
    closeOnFullRender(false),
    purgeDiskCache(false)
{}

MainWindow::MainWindow(vts::Map *map, const AppOptions &appOptions) :
    appOptions(appOptions),
    map(map), window(nullptr),
    dataContext(nullptr), renderContext(nullptr)
{
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    window = SDL_CreateWindow("vts-browser-desktop",
              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600,
              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

    if (!window)
    {
        vts::log(vts::LogLevel::err4, SDL_GetError());
        throw std::runtime_error("Failed to create window");
    }

    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    dataContext = SDL_GL_CreateContext(window);
    renderContext = SDL_GL_CreateContext(window);

    if (!dataContext || !renderContext)
    {
        vts::log(vts::LogLevel::err4, SDL_GetError());
        throw std::runtime_error("Failed to create opengl context");
    }

    SDL_GL_MakeCurrent(window, renderContext);
    SDL_GL_SetSwapInterval(1);

    vts::renderer::loadGlFunctions(&SDL_GL_GetProcAddress);

    {
        int major = 0, minor = 0;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
        std::stringstream s;
        s << "OpenGL version: " << major << "." << minor;
        vts::log(vts::LogLevel::info2, s.str());
    }

    vts::renderer::initialize();

    // load mesh sphere
    {
        meshSphere = std::make_shared<vts::renderer::Mesh>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer(
                                  "data/meshes/sphere.obj"));
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
        meshSphere->load(info, spec);
    }

    // load mesh line
    {
        meshLine = std::make_shared<vts::renderer::Mesh>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer(
                                  "data/meshes/line.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Lines);
        spec.attributes.resize(2);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        spec.attributes[1].enable = true;
        spec.attributes[1].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[1].components = 2;
        spec.attributes[1].offset = sizeof(vts::vec3f);
        vts::ResourceInfo info;
        meshLine->load(info, spec);
    }
}

MainWindow::~MainWindow()
{
    if (map)
        map->renderFinalize();

    vts::renderer::finalize();

    SDL_GL_DeleteContext(renderContext);
    SDL_DestroyWindow(window);
}

void MainWindow::renderFrame()
{
    vts::renderer::RenderOptions &ro = appOptions.render;

    // draw marks
    {
        vts::mat4 viewProj = vts::rawToMat4(map->draws().camera.proj)
                * vts::rawToMat4(map->draws().camera.view);
        const Mark *prev = nullptr;
        for (const Mark &m : marks)
        {
            // view proj
            vts::mat4 mvp = viewProj
                    * vts::translationMatrix(m.coord)
                    * vts::scaleMatrix(map->getPositionViewExtent() * 0.005);
            vts::mat4f mvpf = mvp.cast<float>();
            vts::DrawTask t;
            vts::vec4f c = vts::vec3to4f(m.color, 1);
            for (int i = 0; i < 4; i++)
                t.color[i] = c(i);
            t.mesh = meshSphere;
            memcpy(t.mvp, mvpf.data(), sizeof(t.mvp));
            map->draws().Infographic.push_back(t);
            if (prev)
            {
                t.mesh = meshLine;
                mvp = viewProj * vts::lookAt(m.coord, prev->coord);
                mvpf = mvp.cast<float>();
                memcpy(t.mvp, mvpf.data(), sizeof(t.mvp));
                map->draws().Infographic.push_back(t);
            }
            prev = &m;
        }
    }

    vts::renderer::render(ro, map->draws(), map->celestialBody());
    gui.render(ro.width, ro.height);
}

bool MainWindow::processEvents()
{
    SDL_Event event;
    gui.inputBegin();
    while (SDL_PollEvent(&event))
    {
        // window closing
        if (event.type == SDL_QUIT)
        {
            gui.inputEnd();
            return true;
        }

        // handle gui
        if (gui.input(event))
            continue;

        // add mark
        if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_m)
        {
            Mark mark;
            mark.coord = getWorldPositionFromCursor();
            if (mark.coord(0) == mark.coord(0))
            {
                marks.push_back(mark);
                colorizeMarks();
            }
        }

        // north-up button
        if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_SPACE)
        {
            map->setPositionRotation({0,270,0});
            map->options().navigationType = vts::NavigationType::Quick;
            map->resetNavigationMode();
        }

        // mouse wheel
        if (event.type == SDL_MOUSEWHEEL)
        {
            map->zoom(event.wheel.y);
            map->options().navigationType = vts::NavigationType::Quick;
        }

        // camera jump to double click
        if (event.type == SDL_MOUSEBUTTONDOWN
                && event.button.clicks == 2
                && event.button.state == SDL_BUTTON(SDL_BUTTON_LEFT))
        {
            vts::vec3 posPhys = getWorldPositionFromCursor();
            if (posPhys(0) == posPhys(0))
            {
                double posNav[3];
                map->convert(posPhys.data(), posNav,
                             vts::Srs::Physical, vts::Srs::Navigation);
                map->setPositionPoint(posNav);
                map->options().navigationType = vts::NavigationType::Quick;
            }
        }

        // camera panning or rotating
        if (event.type == SDL_MOUSEMOTION)
        {
            int mode = 0;
            if (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT))
            {
                const Uint8 *keys = SDL_GetKeyboardState(nullptr);
                if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]
                    || keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT])
                    mode = 2;
                else
                    mode = 1;
            }
            else if (event.motion.state & SDL_BUTTON(SDL_BUTTON_RIGHT)
                     || event.motion.state & SDL_BUTTON(SDL_BUTTON_MIDDLE))
                mode = 2;
            double p[3] = { (double)event.motion.xrel,
                            (double)event.motion.yrel, 0 };
            switch (mode)
            {
            case 1:
                map->pan(p);
                map->options().navigationType = vts::NavigationType::Quick;
                break;
            case 2:
                map->rotate(p);
                map->options().navigationType = vts::NavigationType::Quick;
                break;
            }
        }
    }
    gui.inputEnd();
    return false;
}

void MainWindow::run()
{
    if (appOptions.purgeDiskCache)
        map->purgeDiskCache();

    vts::renderer::RenderOptions &ro = appOptions.render;
    SDL_GL_GetDrawableSize(window, &ro.width, &ro.height);
    map->setWindowSize(ro.width, ro.height);

    map->callbacks().loadTexture = std::bind(&vts::renderer::loadTexture,
                std::placeholders::_1, std::placeholders::_2);
    map->callbacks().loadMesh = std::bind(&vts::renderer::loadMesh,
                std::placeholders::_1, std::placeholders::_2);

    setMapConfigPath(appOptions.paths[0]);
    map->renderInitialize();
    gui.initialize(this);

    bool initialPositionSet = false;
    bool shouldClose = false;

    while (!shouldClose)
    {
        if (!initialPositionSet && map->getMapConfigReady())
        {
            initialPositionSet = true;
            if (!appOptions.initialPosition.empty())
            {
                try
                {
                    map->setPositionUrl(appOptions.initialPosition);
                    map->options().navigationType
                            = vts::NavigationType::Instant;
                }
                catch (...)
                {
                    vts::log(vts::LogLevel::warn3,
                             "failed to set initial position");
                }
            }
        }

        shouldClose = processEvents();

        vts::uint32 timeFrameStart = SDL_GetTicks();
        try
        {
            SDL_GL_GetDrawableSize(window, &ro.width, &ro.height);
            map->setWindowSize(ro.width, ro.height);
            map->renderTickPrepare();
            map->renderTickRender();
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
        vts::uint32 timeMapRender = SDL_GetTicks();
        renderFrame();
        vts::uint32 timeAppRender = SDL_GetTicks();

        if (map->statistics().renderTicks % 120 == 0)
        {
            std::string creditLine = std::string() + "vts-browser-desktop: "
                    + map->credits().textFull();
            SDL_SetWindowTitle(window, creditLine.c_str());
        }

        SDL_GL_SwapWindow(window);
        vts::uint32 timeFrameFinish = SDL_GetTicks();

        // temporary workaround for when v-sync is missing
        vts::uint32 duration = timeFrameFinish - timeFrameStart;
        if (duration < 16)
        {
            usleep(16 - duration);
            timeFrameFinish = SDL_GetTicks();
        }

        timingMapProcess = timeMapRender - timeFrameStart;
        timingAppProcess = timeAppRender - timeMapRender;
        timingTotalFrame = timeFrameFinish - timeFrameStart;

        if (appOptions.closeOnFullRender && map->getMapRenderComplete())
            shouldClose = true;
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
    int xx, yy;
    SDL_GetMouseState(&xx, &yy);
    double screenPos[2] = { (double)xx, (double)yy };
    vts::vec3 result;
    vts::renderer::getWorldPosition(screenPos, result.data());
    return result;
}

void MainWindow::setMapConfigPath(const MapPaths &paths)
{
    map->setMapConfigPath(paths.mapConfig, paths.auth, paths.sri);
}

