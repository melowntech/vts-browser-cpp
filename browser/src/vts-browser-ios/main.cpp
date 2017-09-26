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

#include <sstream>
#include <cassert>
#include <vts-browser/map.hpp>
#include <vts-browser/draws.hpp>
#include <vts-browser/options.hpp>
#include <vts-browser/log.hpp>
#include <vts-browser/fetcher.hpp>
#include <vts-renderer/classes.hpp>
#include <vts-renderer/renderer.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

int main(int argc, char *args[])
{
	// initialization

    vts::setLogThreadName("main");
    //vts::setLogMask("I2W2E2");

    vts::log(vts::LogLevel::info3, "initializing SDL library");
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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    SDL_DisplayMode displayMode;
	SDL_GetDesktopDisplayMode(0, &displayMode);

    vts::log(vts::LogLevel::info3, "creating window");
    auto window = SDL_CreateWindow("vts-browser-ios",
              0, 0, displayMode.w, displayMode.h,
              SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN
              | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

    if (!window)
    {
        vts::log(vts::LogLevel::err4, SDL_GetError());
        throw std::runtime_error("Failed to create window");
    }

    vts::log(vts::LogLevel::info3, "creating opengl context");
    auto renderContext = SDL_GL_CreateContext(window);
    
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

	std::shared_ptr<vts::Map> map;
	{
		vts::MapCreateOptions createOptions;
		createOptions.clientId = "vts-browser-ios";
		map = std::make_shared<vts::Map>(createOptions);
    }
    
    map->callbacks().loadTexture = std::bind(&vts::renderer::loadTexture,
                std::placeholders::_1, std::placeholders::_2);
    map->callbacks().loadMesh = std::bind(&vts::renderer::loadMesh,
                std::placeholders::_1, std::placeholders::_2);

	vts::renderer::RenderOptions ro;
	{
		// get default framebuffer object
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if (!SDL_GetWindowWMInfo(window,&info))
        	throw std::runtime_error("SDL_GetWindowWMInfo");
        assert(info.subsystem == SDL_SYSWM_UIKIT);
        ro.targetFrameBuffer = info.info.uikit.framebuffer;
	}
	SDL_GL_GetDrawableSize(window, &ro.width, &ro.height);
	map->setWindowSize(ro.width, ro.height);
    
    map->dataInitialize(vts::Fetcher::create(vts::FetcherOptions()));
    map->renderInitialize();
    
    map->setMapConfigPath("");
    
    // run

	bool shouldClose = false;
    while (!shouldClose)
    {
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
                case SDL_APP_DIDENTERFOREGROUND:
                    SDL_Log("SDL_APP_DIDENTERFOREGROUND");
                    break;
                    
                case SDL_APP_DIDENTERBACKGROUND:
                    SDL_Log("SDL_APP_DIDENTERBACKGROUND");
                    break;
                    
                case SDL_APP_LOWMEMORY:
                    SDL_Log("SDL_APP_LOWMEMORY");
                    break;
                    
                case SDL_APP_TERMINATING:
                case SDL_QUIT:
                    SDL_Log("SDL_APP_TERMINATING");
				    shouldClose = true;
                    break;
                    
                case SDL_APP_WILLENTERBACKGROUND:
                    SDL_Log("SDL_APP_WILLENTERBACKGROUND");
                    break;
                    
                case SDL_APP_WILLENTERFOREGROUND:
                    SDL_Log("SDL_APP_WILLENTERFOREGROUND");
					break;
				}
			}
		}

        SDL_GL_GetDrawableSize(window, &ro.width, &ro.height);
        map->setWindowSize(ro.width, ro.height);
        
        map->dataTick();
        map->renderTickPrepare();
        map->renderTickRender();
        
		vts::renderer::render(ro, map->draws(), map->celestialBody());

        SDL_GL_SwapWindow(window);
    }
    
    // finalization
    
    if (map)
    {
    	map->dataFinalize();
        map->renderFinalize();
    }

    vts::renderer::finalize();
    map.reset();

    SDL_GL_DeleteContext(renderContext);
    SDL_DestroyWindow(window);
    
    return 0;
}



