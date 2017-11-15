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

#include <vts-browser/map.hpp>
#include <vts-browser/draws.hpp>
#include <vts-browser/options.hpp>
#include <vts-browser/log.hpp>
#include <vts-browser/fetcher.hpp>
#include <vts-renderer/renderer.hpp>
#include <SDL2/SDL.h>

SDL_Window *window;
SDL_GLContext renderContext;
std::shared_ptr<vts::Map> map;
vts::renderer::RenderOptions renderOptions;
bool shouldClose = false;

void updateResolution()
{
    if (!map || !window)
        return;
    SDL_GL_GetDrawableSize(window, &renderOptions.width, &renderOptions.height);
    map->setWindowSize(renderOptions.width, renderOptions.height);
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
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0); // we do not need default depth buffer, the rendering library uses its own
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3); // use OpenGL version 3.0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    // create window
    vts::log(vts::LogLevel::info3, "Creating window");
    {
        // the created window should fill the whole screen
        SDL_DisplayMode displayMode;
        SDL_GetDesktopDisplayMode(0, &displayMode);
        window = SDL_CreateWindow("vts-browser-minimal",
                  0, 0, displayMode.w, displayMode.h,
                  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    }
    if (!window)
    {
        vts::log(vts::LogLevel::err4, SDL_GetError());
        throw std::runtime_error("Failed to create window");
    }

    // create OpenGL context
    vts::log(vts::LogLevel::info3, "Creating OpenGL context");
    renderContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, renderContext);
    SDL_GL_SetSwapInterval(1);

    // notify the vts renderer library on how to load OpenGL function pointers
    vts::renderer::loadGlFunctions(&SDL_GL_GetProcAddress);
    // and initialize the library, which will load required shaders and other local files
    vts::renderer::initialize();

    // create instance of the vts::Map class
    map = std::make_shared<vts::Map>(vts::MapCreateOptions());

    // set required callbacks for creating mesh and texture resources
    map->callbacks().loadTexture = std::bind(&vts::renderer::loadTexture,
                std::placeholders::_1, std::placeholders::_2);
    map->callbacks().loadMesh = std::bind(&vts::renderer::loadMesh,
                std::placeholders::_1, std::placeholders::_2);

    // initialize the resource processing with default fetcher
    map->dataInitialize(vts::Fetcher::create(vts::FetcherOptions()));

    // initialize the rendering part of the map
    updateResolution();
    map->renderInitialize();

    // configure an url to the map that should be displayed
    map->setMapConfigPath("https://cdn.melown.com/mario/store/melown2015/map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json");

    // keep processing window events
    while (!shouldClose)
    {
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
                    if (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT))
                    {
                        double p[3] = { (double)event.motion.xrel,
                                    (double)event.motion.yrel, 0 };
                        map->pan(p);
                    }
                    break;
                }
            }
        }

        // update the map (both resources and rendering)
        updateResolution();
        map->dataTick(); // update downloads
        map->renderTickPrepare(); // update navigation etc.
        map->renderTickRender(); // prepare the rendering data

        // actually render the map
        vts::renderer::render(renderOptions, map->draws(),
                              map->celestialBody());

        // present the rendered image to the screen
        SDL_GL_SwapWindow(window);
    }

    // release all rendering related data
    vts::renderer::finalize();

    // release the map
    if (map)
    {
        map->dataFinalize();
        map->renderFinalize();
        map.reset();
    }

    // free the OpenGL context
    if (renderContext)
    {
        SDL_GL_DeleteContext(renderContext);
        renderContext = nullptr;
    }

    // release the window
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    return 0;
}




