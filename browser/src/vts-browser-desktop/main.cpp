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

#include <cstdio>
#include <cstdlib>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <optick.h>

#include "mainWindow.hpp"
#include "dataThread.hpp"
#include "programOptions.hpp"

void initializeSdl(struct SDL_Window *&window,
    void *&renderContext, void *&dataContext)
{
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        vts::log(vts::LogLevel::err4, SDL_GetError());
        throw std::runtime_error("Failed to initialize SDL");
    }

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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    {
        window = SDL_CreateWindow("vts-browser-desktop",
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
}

void finalizeSdl(struct SDL_Window *window,
    void *renderContext, void *dataContext)
{
    SDL_GL_DeleteContext(dataContext);
    SDL_GL_DeleteContext(renderContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    // release build -> catch exceptions and print them to stderr
    // debug build -> let the debugger handle the exceptions
#ifdef NDEBUG
    try
    {
#endif
        OPTICK_THREAD("main");
        vts::setLogThreadName("main");
        vts::setLogFile("vts-browser-desktop.log");
        //vts::setLogMask("I2W2E2");
        //vts::setLogMask("D");

        vts::MapCreateOptions createOptions;
        createOptions.clientId = "vts-browser-desktop";
        vts::MapRuntimeOptions mapOptions;
        vts::CameraOptions camOptions;
        vts::NavigationOptions navOptions;
        mapOptions.targetResourcesMemoryKB = 512 * 1024;
        vts::FetcherOptions fetcherOptions;
        AppOptions appOptions;
        vts::renderer::RenderOptions renderOptions;
        renderOptions.colorToTexture = true;
        if (!programOptions(createOptions, mapOptions, fetcherOptions,
                            camOptions, navOptions,
                            renderOptions, appOptions, argc, argv))
            return 0;

        struct SDL_Window *window = nullptr;
        void *renderContext = nullptr;
        void *dataContext = nullptr;
        initializeSdl(window, renderContext, dataContext);
        vts::renderer::loadGlFunctions(&SDL_GL_GetProcAddress);

        // dpi
        {
            float vdpi = mapOptions.pixelsPerInch;
            if (SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(window),
                nullptr, nullptr, &vdpi) == 0)
                mapOptions.pixelsPerInch = vdpi;
            else
                vts::log(vts::LogLevel::warn3,
                    "Failed to retrieve monitor DPI");
        }

        // vts map and main loop entry
        {
            vts::Map map(createOptions,
                vts::Fetcher::create(fetcherOptions));
            auto camera = map.createCamera();
            auto navigation = camera->createNavigation();
            map.options() = mapOptions;
            camera->options() = camOptions;
            navigation->options() = navOptions;
            DataThread data(window, dataContext, &map);
            MainWindow main(window, renderContext,
                &map, camera.get(), navigation.get(),
                appOptions, renderOptions);
            data.start();
            main.run();
        }

        finalizeSdl(window, renderContext, dataContext);
        return 0;

#ifdef NDEBUG
    }
    catch(const std::exception &e)
    {
        std::stringstream s;
        s << "Exception <" << e.what() << ">";
        vts::log(vts::LogLevel::err4, s.str());
        return 1;
    }
    catch(const char *e)
    {
        std::stringstream s;
        s << "Exception <" << e << ">";
        vts::log(vts::LogLevel::err4, s.str());
        return 1;
    }
    catch(...)
    {
        vts::log(vts::LogLevel::err4, "Unknown exception.");
        return 1;
    }
#endif
}

