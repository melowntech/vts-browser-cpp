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

#include <vts-browser/log.hpp>
#include <vts-browser/map.hpp>
#include <vts-browser/mapOptions.hpp>
#include <vts-browser/mapCallbacks.hpp>
#include <vts-browser/camera.hpp>
#include <vts-browser/navigation.hpp>
#include <vts-browser/navigationOptions.hpp>
#include <vts-browser/math.hpp>
#include <vts-renderer/renderer.hpp>

#include <chrono>
#include <thread>
#include <iostream>
#include <iomanip>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

SDL_Window *window;
SDL_GLContext renderContext;
SDL_GLContext dataContext;
std::shared_ptr<vts::Map> map;
std::shared_ptr<vts::Camera> cam;
std::shared_ptr<vts::Navigation> nav;
std::shared_ptr<vts::renderer::RenderContext> context;
std::shared_ptr<vts::renderer::RenderView> view;
std::thread dataThread;
bool shouldClose = false;

void dataEntry()
{
    vts::setLogThreadName("data");
    SDL_GL_MakeCurrent(window, dataContext);
    vts::renderer::installGlDebugCallback();
    map->dataAllRun();
    SDL_GL_DeleteContext(dataContext);
    dataContext = nullptr;
}

void updateResolution()
{
    int w = 0, h = 0;
    SDL_GL_GetDrawableSize(window, &w, &h);
    auto &ro = view->options();
    ro.width = w;
    ro.height = h;
    cam->setViewportSize(ro.width, ro.height);
}

int main(int, char *[])
{
    vts::log(vts::LogLevel::info3, "Initializing SDL library");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        vts::log(vts::LogLevel::err4, SDL_GetError());
        throw std::runtime_error("Failed to initialize SDL");
    }

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    vts::log(vts::LogLevel::info3, "Creating window");
    {
        window = SDL_CreateWindow("test-smoothness",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            800, 600,
            SDL_WINDOW_MAXIMIZED | SDL_WINDOW_OPENGL
            | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    }
    if (!window)
    {
        vts::log(vts::LogLevel::err4, SDL_GetError());
        throw std::runtime_error("Failed to create window");
    }

    vts::log(vts::LogLevel::info3, "Creating OpenGL contexts");
    dataContext = SDL_GL_CreateContext(window);
    renderContext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(0); // disable v-sync!

    map = std::make_shared<vts::Map>();
    vts::renderer::loadGlFunctions(&SDL_GL_GetProcAddress);
    context = std::make_shared<vts::renderer::RenderContext>();
    context->bindLoadFunctions(map.get());
    cam = map->createCamera();
    nav = cam->createNavigation();
    view = context->createView(cam.get());
    updateResolution();
    dataThread = std::thread(&dataEntry);
    map->renderInitialize();

    map->setMapconfigPath("https://cdn.melown.com/mario/store/melown2015/"
            "map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json");

    map->callbacks().mapconfigReady = []()
    {
        nav->options().navigationType = vts::NavigationType::Instant;
        nav->setPositionUrl(
            "obj,14.42865,50.09448,fixed,227.801"
            ",-149.862,-21.2,0,267.127,45");
        cam->renderUpdate(); // apply the position
        nav->options().navigationType = vts::NavigationType::Quick;
        map->callbacks().mapconfigReady = {};
    };

    auto lastTime = std::chrono::high_resolution_clock::now();
    double accumulatedTime = 0;
    vts::vec3 lastPosition = vts::nan3();
    while (!shouldClose)
    {
        // closing
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_APP_TERMINATING:
                case SDL_QUIT:
                    shouldClose = true;
                    break;
                }
            }
        }

        // measure elapsed time
        auto currentTime = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double>(
            currentTime - lastTime).count();
        accumulatedTime += duration;
        lastTime = currentTime;

        // navigation
        nav->pan({ 0.0, duration * 15, 0.0 });

        // update and render
        updateResolution();
        map->renderUpdate(duration);
        cam->renderUpdate();
        view->render();
        SDL_GL_SwapWindow(window);

        // simulated fps fluctuations
        std::this_thread::sleep_for(std::chrono::milliseconds(
            std::sin(accumulatedTime * 0.1) < 0 ? 50 : 0));

        // show statistics
        if (map->getMapconfigReady())
        {
            vts::vec3 currentPosition;
            nav->getPoint(currentPosition.data());
            map->convert(currentPosition.data(), currentPosition.data(),
                vts::Srs::Navigation, vts::Srs::Physical);
            double distance = vts::length(
                vts::vec3(currentPosition - lastPosition));
            lastPosition = currentPosition;

            std::stringstream ss;
            ss << std::fixed << std::setprecision(3) << std::showpoint
                << "frame duration: " << duration << " s"
                << ", distance: " << distance << " m"
                << ", speed: " << (distance / duration) << " m/s";
            SDL_SetWindowTitle(window, ss.str().c_str());
        }
    }

    // release all
    view.reset();
    nav.reset();
    cam.reset();
    map->renderFinalize();
    dataThread.join();
    context.reset();
    map.reset();

    SDL_GL_DeleteContext(renderContext);
    renderContext = nullptr;
    SDL_DestroyWindow(window);
    window = nullptr;

    return 0;
}




