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
#include <vts-browser/camera.hpp>
#include <vts-browser/navigation.hpp>
#include <vts-renderer/renderer.hpp>

#include <emscripten.h>
#include <SDL2/SDL.h>

SDL_Window *window;
SDL_GLContext renderContext;
std::shared_ptr<vts::Map> map;
std::shared_ptr<vts::Camera> cam;
std::shared_ptr<vts::Navigation> nav;
std::shared_ptr<vts::renderer::RenderContext> context;
std::shared_ptr<vts::renderer::RenderView> view;
uint32 lastRenderTime;

void updateResolution()
{
    int w = 0, h = 0;
    SDL_GL_GetDrawableSize(window, &w, &h);
    auto &ro = view->options();
    ro.width = w;
    ro.height = h;
    cam->setViewportSize(ro.width, ro.height);
}

void loopIteration()
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
                emscripten_cancel_main_loop();
                return;
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

    // update and render
    updateResolution();
    map->dataUpdate();
    uint32 currentRenderTime = SDL_GetTicks();
    map->renderUpdate((currentRenderTime - lastRenderTime) * 1e-3);
    lastRenderTime = currentRenderTime;
    cam->renderUpdate();
    view->render();
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

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // create window
    vts::log(vts::LogLevel::info3, "Creating window");
    {
        window = SDL_CreateWindow("vts-browser-wasm",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            800, 600, SDL_WINDOW_OPENGL);
    }
    if (!window)
    {
        vts::log(vts::LogLevel::err4, SDL_GetError());
        throw std::runtime_error("Failed to create window");
    }

    // create OpenGL context
    vts::log(vts::LogLevel::info3, "Creating OpenGL context");
    renderContext = SDL_GL_CreateContext(window);
    vts::renderer::loadGlFunctions(&SDL_GL_GetProcAddress);

    // initialize browser and renderer
    vts::log(vts::LogLevel::info3, "Creating browser map");
    map = std::make_shared<vts::Map>();
    context = std::make_shared<vts::renderer::RenderContext>();
    context->bindLoadFunctions(map.get());
    cam = map->createCamera();
    nav = cam->createNavigation();
    view = context->createView(cam.get());
    updateResolution();

    map->setMapconfigPath("https://cdn.melown.com/mario/store/melown2015/"
            "map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json");

    // run the game loop
    vts::log(vts::LogLevel::info3, "Starting the game loop");
    lastRenderTime = SDL_GetTicks();
    emscripten_set_main_loop(&loopIteration, 0, true);
    return 0;
}


