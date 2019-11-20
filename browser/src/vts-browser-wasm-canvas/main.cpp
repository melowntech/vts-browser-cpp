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
#include <emscripten/html5.h>

std::shared_ptr<vts::Map> map;
std::shared_ptr<vts::Camera> cam;
std::shared_ptr<vts::Navigation> nav;
std::shared_ptr<vts::renderer::RenderContext> context;
std::shared_ptr<vts::renderer::RenderView> view;
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;

void updateResolution()
{
    int w = 0, h = 0;
    //emscripten_get_canvas_element_size(nullptr, &w, &h);
    emscripten_webgl_get_drawing_buffer_size(ctx, &w, &h);
    auto &ro = view->options();
    ro.width = w;
    ro.height = h;
    cam->setViewportSize(ro.width, ro.height);
}

void loopIteration()
{
    // process events
    // todo

    // update and render
    updateResolution();
    map->dataUpdate();
    map->renderUpdate(0.1);
    cam->renderUpdate();
    view->render();
}

int main(int, char *[])
{
    // create OpenGL context
    vts::log(vts::LogLevel::info3, "Creating OpenGL context");
    {
        EmscriptenWebGLContextAttributes attr;
        emscripten_webgl_init_context_attributes(&attr);
        attr.alpha = attr.depth = attr.stencil = 0;
        attr.antialias = 0;
        attr.majorVersion = 2;
        attr.minorVersion = 0;
        ctx = emscripten_webgl_create_context("#canvas", &attr);
    }
    emscripten_webgl_make_context_current(ctx);
    vts::log(vts::LogLevel::info2, "Initializing OpenGL function pointers");
    vts::renderer::loadGlFunctions(&emscripten_webgl_get_proc_address);

    // initialize browser and renderer
    vts::log(vts::LogLevel::info3, "Creating browser map");
    map = std::make_shared<vts::Map>();
    context = std::make_shared<vts::renderer::RenderContext>();
    context->bindLoadFunctions(map.get());
    cam = map->createCamera();
    nav = cam->createNavigation();
    view = context->createView(cam.get());
    updateResolution();
    map->dataInitialize();
    map->renderInitialize();

    map->setMapconfigPath("https://cdn.melown.com/mario/store/melown2015/"
            "map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json");

    // run the game loop
    vts::log(vts::LogLevel::info3, "Starting the game loop");
    emscripten_set_main_loop(&loopIteration, 0, true);
    return 0;
}


