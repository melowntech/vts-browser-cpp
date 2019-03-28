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

#include <vts-browser/map.h>
#include <vts-browser/camera.h>
#include <vts-browser/navigation.h>
#include <vts-browser/log.h>
#include <vts-renderer/renderer.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

SDL_Window *window;
SDL_GLContext renderContext;
vtsHRenderer render;
vtsHMap map;
vtsHCamera cam;
vtsHNavigation nav;
bool shouldClose = false;

void check()
{
    sint32 c = vtsErrCode();
    if (c == 0)
        return;
    vtsLog(vtsLogLevelErr4, vtsErrMsg());
    vtsLog(vtsLogLevelErr4, vtsErrCodeToName(c));
    exit(-1);
}

void updateResolution()
{
    int w = 0, h = 0;
    SDL_GL_GetDrawableSize(window, &w, &h);
    vtsCRenderOptionsBase *ro = vtsRendererOptions(render);
    ro->width = w;
    ro->height = h;
    vtsCameraSetViewportSize(cam, ro->width, ro->height);
    check();
}

int main()
{
    // initialize SDL
    vtsLog(vtsLogLevelInfo3, "Initializing SDL library");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        vtsLog(vtsLogLevelErr4, SDL_GetError());
        exit(-1);
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

    // create window
    vtsLog(vtsLogLevelInfo3, "Creating window");
    {
        window = SDL_CreateWindow("vts-browser-minimal-c",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            800, 600,
            SDL_WINDOW_MAXIMIZED | SDL_WINDOW_OPENGL
            | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    }
    if (!window)
    {
        vtsLog(vtsLogLevelErr4, SDL_GetError());
        exit(-1);
    }

    // create OpenGL context
    vtsLog(vtsLogLevelInfo3, "Creating OpenGL context");
    renderContext = SDL_GL_CreateContext(window);
    // bind the OpenGL context to current thread
    SDL_GL_MakeCurrent(window, renderContext);
    SDL_GL_SetSwapInterval(1); // enable v-sync

    // notify the vts renderer library on how to load OpenGL function pointers
    vtsLoadGlFunctions(&SDL_GL_GetProcAddress);
    check();

    // and initialize the renderer library
    // this will load required shaders and other local files
    render = vtsRendererCreate();
    check();
    vtsRendererInitialize(render);
    check();

    // create instance of the vts::Map class
    map = vtsMapCreate("", NULL);
    check();

    // acquire handles for camera and navigation
    cam = vtsCameraCreate(map);
    check();
    nav = vtsNavigationCreate(cam);
    check();

    // set required callbacks for creating mesh and texture resources
    vtsRendererBindLoadFunctions(render, map);
    check();

    // initialize the resource processing with default fetcher
    vtsMapDataInitialize(map);
    check();

    // initialize the render preparation component of the map
    updateResolution();
    vtsMapRenderInitialize(map);
    check();

    // configure an url to the map that should be displayed
    vtsMapSetConfigPaths(map,
            "https://cdn.melown.com/mario/store/melown2015/"
            "map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json",
            "");
    check();

    // acquire current time to measure how long each frame takes
    uint32 lastRenderTime = SDL_GetTicks();

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
                {
                    // relative mouse position
                    double p[3] = { (double)event.motion.xrel,
                                (double)event.motion.yrel, 0 };
                    if (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT))
                    {
                        vtsNavigationPan(nav, p);
                        check();
                    }
                    if (event.motion.state & SDL_BUTTON(SDL_BUTTON_RIGHT))
                    {
                        vtsNavigationRotate(nav, p);
                        check();
                    }
                } break;
                case SDL_MOUSEWHEEL:
                    vtsNavigationZoom(nav, event.wheel.y);
                    check();
                    break;
                }
            }
        }

        // update downloads
        vtsMapDataUpdate(map);
        check();

        // check if the window has been resized
        updateResolution();

        // update navigation etc.
        uint32 currentRenderTime = SDL_GetTicks();
        vtsMapRenderUpdate(map, (currentRenderTime - lastRenderTime) * 1e-3);
        check();
        vtsCameraRenderUpdate(cam);
        check();
        lastRenderTime = currentRenderTime;

        // actually render the map
        vtsRendererRender(render, cam);
        check();

        // present the rendered image to the screen
        SDL_GL_SwapWindow(window);
    }

    // release all rendering related data
    vtsRendererFinalize(render);
    check();

    // release the map
    vtsMapDataFinalize(map);
    check();
    vtsRendererFinalize(render);
    check();
    vtsNavigationDestroy(nav);
    vtsCameraDestroy(cam);
    vtsMapDestroy(map);
    vtsRendererDestroy(render);

    // free the OpenGL context
    if (renderContext)
    {
        SDL_GL_DeleteContext(renderContext);
        renderContext = NULL;
    }

    // release the window
    if (window)
    {
        SDL_DestroyWindow(window);
        window = NULL;
    }

    return 0;
}




