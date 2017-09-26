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

SDL_Window *window;
SDL_GLContext renderContext;
std::shared_ptr<vts::Map> map;
vts::renderer::RenderOptions renderOptions;
bool shouldClose = false;

int handleAppEvents(void *, SDL_Event *event);

void updateResolution()
{
	if (!map || !window)
		return;
    SDL_GL_GetDrawableSize(window, &renderOptions.width, &renderOptions.height);
    map->setWindowSize(renderOptions.width, renderOptions.height);

    std::stringstream s;
    s << "Resolution: " << renderOptions.width << " x " << renderOptions.height;
    vts::log(vts::LogLevel::info2, s.str());
}

void initialize()
{
    vts::log(vts::LogLevel::info3, "Initializing SDL library");
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        vts::log(vts::LogLevel::err4, SDL_GetError());
        throw std::runtime_error("Failed to initialize SDL");
    }
    SDL_SetEventFilter(&handleAppEvents, nullptr);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	{
		vts::log(vts::LogLevel::info3, "Creating window");
		SDL_DisplayMode displayMode;
		SDL_GetDesktopDisplayMode(0, &displayMode);
		window = SDL_CreateWindow("vts-browser-ios",
		          0, 0, displayMode.w, displayMode.h,
		          SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN
		          | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
		          | SDL_WINDOW_ALLOW_HIGHDPI);
	}

    if (!window)
    {
        vts::log(vts::LogLevel::err4, SDL_GetError());
        throw std::runtime_error("Failed to create window");
    }

    vts::log(vts::LogLevel::info3, "Creating opengl context");
    renderContext = SDL_GL_CreateContext(window);
    
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

	{
		vts::MapCreateOptions createOptions;
		createOptions.clientId = "vts-browser-ios";
		map = std::make_shared<vts::Map>(createOptions);
    }
    
    map->callbacks().loadTexture = std::bind(&vts::renderer::loadTexture,
                std::placeholders::_1, std::placeholders::_2);
    map->callbacks().loadMesh = std::bind(&vts::renderer::loadMesh,
                std::placeholders::_1, std::placeholders::_2);

	{
		// get default framebuffer object
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if (!SDL_GetWindowWMInfo(window,&info))
        	throw std::runtime_error("SDL_GetWindowWMInfo");
        assert(info.subsystem == SDL_SYSWM_UIKIT);
        renderOptions.targetFrameBuffer = info.info.uikit.framebuffer;
	}
	
	updateResolution();
    
    map->dataInitialize(vts::Fetcher::create(vts::FetcherOptions()));
    map->renderInitialize();
    
    map->options().traverseMode = vts::TraverseMode::Hierarchical;
    map->options().maxTexelToPixelScale *= 3;
}

void finalize()
{
    vts::renderer::finalize();
    
    if (map)
    {
    	map->dataFinalize();
        map->renderFinalize();
    	map.reset();
    }

	if (renderContext)
	{
	    SDL_GL_DeleteContext(renderContext);
	    renderContext = nullptr;
    }
    
    if (window)
    {
	    SDL_DestroyWindow(window);
	    window = nullptr;
	}
}

int handleAppEvents(void *, SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_APP_TERMINATING:
    	vts::log(vts::LogLevel::info3, "Event: terminating");
    	shouldClose = true;
    	finalize();
        return 0;
    case SDL_APP_LOWMEMORY:
    	vts::log(vts::LogLevel::info3, "Event: low memory");
        return 0;
    case SDL_APP_WILLENTERBACKGROUND:
    	vts::log(vts::LogLevel::info3, "Event: will enter background");
        return 0;
    case SDL_APP_DIDENTERBACKGROUND:
    	vts::log(vts::LogLevel::info3, "Event: did enter background");
        return 0;
    case SDL_APP_WILLENTERFOREGROUND:
    	vts::log(vts::LogLevel::info3, "Event: will enter foreground");
        return 0;
    case SDL_APP_DIDENTERFOREGROUND:
    	vts::log(vts::LogLevel::info3, "Event: did enter foreground");
        return 0;
    default:
        return 1;
    }
}

int main(int argc, char *args[])
{
    vts::setLogThreadName("main");
    //vts::setLogMask("I2W2E2");
    
	initialize();
    map->setMapConfigPath("https://cdn.melown.com/mario/store/melown2015/map-config/melown/Intergeo-Melown-Earth/mapConfig.json");
    
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
				}
			}
		}

		updateResolution();
        
        map->dataTick();
        map->renderTickPrepare();
        map->renderTickRender();
        
		vts::renderer::render(renderOptions, map->draws(), map->celestialBody());

        SDL_GL_SwapWindow(window);
    }
    
    finalize();
    return 0;
}



