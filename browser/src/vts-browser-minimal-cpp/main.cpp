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
#include <vts-browser/camera.hpp>
#include <vts-browser/navigation.hpp>
#include <vts-renderer/renderer.hpp>

#include <thread>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

SDL_Window *window;
SDL_GLContext renderContext;
SDL_GLContext dataContext;
std::shared_ptr<vts::renderer::Renderer> render;
std::shared_ptr<vts::Map> map;
std::shared_ptr<vts::Camera> cam;
std::shared_ptr<vts::Navigation> nav;
std::thread dataThread;
bool shouldClose = false;

void dataEntry()
{
    vts::setLogThreadName("data");

    // the browser uses separate thread for uploading resources to gpu memory
    //   this thread must have access to an OpenGL context
    //   and the context must be shared with the one used for rendering
    SDL_GL_MakeCurrent(window, dataContext);
    vts::renderer::installGlDebugCallback();

    // this will block until map->renderFinalize
    //   is called in the rendering thread
    map->dataAllRun();

    SDL_GL_DeleteContext(dataContext);
    dataContext = nullptr;
}

void updateResolution()
{
    int w = 0, h = 0;
    SDL_GL_GetDrawableSize(window, &w, &h);
    auto &ro = render->options();
    ro.width = w;
    ro.height = h;
    cam->setViewportSize(ro.width, ro.height);
}

int main(int, char *[])
{
    // initialize SDL
    vts::log(vts::LogLevel::info3, "Initializing SDL library");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        vts::log(vts::LogLevel::err4, SDL_GetError());
        throw std::runtime_error("Failed to initialize SDL");
    }

    // configure parameters for OpenGL context
    // we do not need default depth buffer, the rendering library uses its own
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    // use OpenGL version 3.3 core profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    // enable sharing resources between multiple OpenGL contexts
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    // create window
    vts::log(vts::LogLevel::info3, "Creating window");
    {
        window = SDL_CreateWindow("vts-browser-minimal-cpp",
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

    // create OpenGL contexts
    vts::log(vts::LogLevel::info3, "Creating OpenGL contexts");
    dataContext = SDL_GL_CreateContext(window);
    renderContext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // enable v-sync

    // create instance of the vts::Map class
    map = std::make_shared<vts::Map>();

    // make vts renderer library load OpenGL function pointers
    // this calls installGlDebugCallback for the current context too
    vts::renderer::loadGlFunctions(&SDL_GL_GetProcAddress);

    // initialize the renderer library
    // this will load required shaders and other local files
    render = std::make_shared<vts::renderer::Renderer>();
    render->initialize();

    // set required callbacks for creating mesh and texture resources
    render->bindLoadFunctions(map.get());

    // launch the data thread
    dataThread = std::thread(&dataEntry);

    // create a camera and acquire its navigation handle
    cam = map->camera();
    nav = cam->navigation();

    // initialize the map for rendering
    updateResolution();
    map->renderInitialize();

    // pass a mapconfig url to the map
    map->setMapconfigPath("https://cdn.melown.com/mario/store/melown2015/"
            "map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json");

    // acquire current time (for measuring how long each frame takes)
    uint32 lastRenderTime = SDL_GetTicks();

    // main event loop
    while (!shouldClose)
    {
        // process events
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                // handle window close
                case SDL_APP_TERMINATING:
                case SDL_QUIT:
                    shouldClose = true;
                    break;
                // handle mouse events
                case SDL_MOUSEMOTION:
                {
                    // relative mouse position
                    double p[3] = { (double)event.motion.xrel,
                                (double)event.motion.yrel, 0 };
                    if (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT))
                        nav->pan(p);
                    if (event.motion.state & SDL_BUTTON(SDL_BUTTON_RIGHT))
                        nav->rotate(p);
                } break;
                case SDL_MOUSEWHEEL:
                    nav->zoom(event.wheel.y);
                    break;
                }
            }
        }

        // update navigation etc.
        updateResolution();
        uint32 currentRenderTime = SDL_GetTicks();
        map->renderUpdate((currentRenderTime - lastRenderTime) * 1e-3);
        cam->renderUpdate();
        lastRenderTime = currentRenderTime;

        // actually render the map
        render->render(cam.get());
        SDL_GL_SwapWindow(window);
    }

    // release all
    nav.reset();
    cam.reset();
    map->renderFinalize();
    dataThread.join();
    render->finalize();
    render.reset();
    map.reset();

    SDL_GL_DeleteContext(renderContext);
    renderContext = nullptr;
    SDL_DestroyWindow(window);
    window = nullptr;

    return 0;
}




