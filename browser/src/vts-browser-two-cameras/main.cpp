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
#include <vts-browser/mapOptions.hpp>
#include <vts-browser/log.hpp>
#include <vts-browser/camera.hpp>
#include <vts-browser/navigation.hpp>
#include <vts-renderer/renderer.hpp>

#include <thread>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

SDL_Window *window;
SDL_GLContext renderContext;
SDL_GLContext dataContext;
std::shared_ptr<vts::Map> map;
std::shared_ptr<vts::Camera> cam1;
std::shared_ptr<vts::Camera> cam2;
std::shared_ptr<vts::Navigation> nav1;
std::shared_ptr<vts::Navigation> nav2;
std::shared_ptr<vts::Navigation> navLast;
std::shared_ptr<vts::renderer::RenderContext> context;
std::shared_ptr<vts::renderer::RenderView> view1;
std::shared_ptr<vts::renderer::RenderView> view2;
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

void setOptions(vts::renderer::RenderOptions &ro, uint32 w, uint32 h)
{
    ro.width = w / 2;
    ro.height = h;
    ro.targetViewportX = 0;
    ro.targetViewportY = 0;
    ro.targetViewportW = w / 2;
    ro.targetViewportH = h;
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
        window = SDL_CreateWindow("test-two-cameras",
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

    vts::log(vts::LogLevel::info3, "Creating OpenGL context");
    dataContext = SDL_GL_CreateContext(window);
    renderContext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1);
    vts::renderer::loadGlFunctions(&SDL_GL_GetProcAddress);

    map = std::make_shared<vts::Map>();
    context = std::make_shared<vts::renderer::RenderContext>();
    context->bindLoadFunctions(map.get());

    dataThread = std::thread(&dataEntry);

    cam1 = map->createCamera();
    cam2 = map->createCamera();
    nav1 = cam1->createNavigation();
    nav2 = cam2->createNavigation();
    view1 = context->createView(cam1.get());
    view2 = context->createView(cam2.get());

    map->setMapconfigPath("https://cdn.melown.com/mario/store/melown2015/"
            "map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json");

    uint32 lastRenderTime = SDL_GetTicks();
    while (!shouldClose)
    {
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
                case SDL_MOUSEMOTION:
                {
                    navLast = event.motion.x < (sint32)view1->options().width
                        ? nav1 : nav2;
                    double p[3] = { (double)event.motion.xrel,
                                (double)event.motion.yrel, 0 };
                    if (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT))
                        navLast->pan(p);
                    if (event.motion.state & SDL_BUTTON(SDL_BUTTON_RIGHT))
                        navLast->rotate(p);
                } break;
                case SDL_MOUSEWHEEL:
                    if (navLast)
                        navLast->zoom(event.wheel.y);
                    break;
                }
            }
        }

        int w, h;
        SDL_GL_GetDrawableSize(window, &w, &h);
        cam1->setViewportSize(w / 2, h);
        cam2->setViewportSize(w / 2, h);
        {
            double mat[16];
            cam1->getView(mat);
            cam2->setView(mat);
            cam1->getProj(mat);
            cam2->setProj(mat);
        }

        uint32 currentRenderTime = SDL_GetTicks();
        map->renderUpdate((currentRenderTime - lastRenderTime) * 1e-3);
        cam1->renderUpdate();
        cam2->renderUpdate();
        lastRenderTime = currentRenderTime;

        {
            auto &ro = view1->options();
            setOptions(ro, w, h);
        }
        {
            auto &ro = view2->options();
            setOptions(ro, w, h);
            ro.targetViewportX = w / 2;
        }

        view1->render();
        view2->render();

        SDL_GL_SwapWindow(window);
    }

    view1.reset();
    view2.reset();
    nav1.reset();
    nav2.reset();
    cam1.reset();
    cam2.reset();
    map->renderFinalize();
    dataThread.join();
    map.reset();
    context.reset();
    SDL_GL_DeleteContext(renderContext);
    renderContext = nullptr;
    SDL_DestroyWindow(window);
    window = nullptr;

    return 0;
}




