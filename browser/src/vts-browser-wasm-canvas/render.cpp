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

#include "render.hpp"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/threading.h>

#include <future> // our life is pointless without it
#include <mutex>

//#define RENDER_EXPLICIT_SWAP

std::shared_ptr<vts::renderer::RenderContext> context;
std::shared_ptr<vts::renderer::RenderView> view;
std::mutex mutex;
DrawsQueue drawsQueue, drawsQueue2;
uint32 displayWidth, displayHeight;
DurationBuffer durationRenderFrame, durationRenderData,
    durationRenderRender, durationRenderSwap;

namespace
{

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;
pthread_t thread;
std::promise<void> threadInitialized;

void updateResolution()
{
    int w = 0, h = 0;
    emscripten_webgl_get_drawing_buffer_size(ctx, &w, &h);
    auto &ro = view->options();
    ro.width = w;
    ro.height = h;
    displayWidth = w;
    displayHeight = h;
}

void loopIteration()
{
    //vts::log(vts::LogLevel::info2, "Render loop iteration");

    TimerPoint a = now();
    map->dataUpdate(); // outside lock

    TimerPoint b = now();
    std::unique_ptr<vts::renderer::RenderDraws> rd;
    drawsQueue.waitPop(rd);

    TimerPoint c = now();
    updateResolution();
    {
        std::lock_guard<std::mutex> lock(mutex);
        view->render(rd.get());
    }
    drawsQueue2.push(std::move(rd));

    TimerPoint d = now();
#ifdef RENDER_EXPLICIT_SWAP
    emscripten_webgl_commit_frame();
#endif

    TimerPoint e = now();
    durationRenderFrame.update(a, e);
    durationRenderData.update(a, b);
    durationRenderRender.update(c, d);
    durationRenderSwap.update(d, e);
}

void *renderThreadEntry(void*)
{
    vts::setLogThreadName("render");

    // create OpenGL context
    vts::log(vts::LogLevel::info3, "Creating OpenGL context");
    {
        EmscriptenWebGLContextAttributes attr;
        emscripten_webgl_init_context_attributes(&attr);
        attr.alpha = attr.depth = attr.stencil = attr.antialias = 0; // we have our own render target
        attr.majorVersion = 2; // WebGL 2.0
        attr.minorVersion = 0;
#ifdef RENDER_EXPLICIT_SWAP
        attr.explicitSwapControl = true;
#endif
        ctx = emscripten_webgl_create_context("#display", &attr);
        if (!ctx)
        {
            threadInitialized.set_exception(
                std::make_exception_ptr(
                std::runtime_error("Failed to create OpenGL context")));
            return nullptr;
        }
    }
    emscripten_webgl_make_context_current(ctx);
    vts::log(vts::LogLevel::info3, "Initializing OpenGL function pointers");
    vts::renderer::loadGlFunctions(&emscripten_webgl_get_proc_address);

    // create vts renderer
    vts::log(vts::LogLevel::info3, "Creating vts render context");
    context = std::make_shared<vts::renderer::RenderContext>();
    context->bindLoadFunctions(map.get());
    view = context->createView(cam.get());
    updateResolution();
    threadInitialized.set_value();

    // create 3 buffers:
    // 1. buffer is being written to
    // 2. buffer is being rendered from
    // 3. buffer is transient to give threads some additional space to breathe
    for (int i = 0; i < 3; i++)
        drawsQueue2.push(std::make_unique<vts::renderer::RenderDraws>());

    // run rendering loop
    vts::log(vts::LogLevel::info3, "Starting rendering loop");
#ifdef RENDER_EXPLICIT_SWAP
    while (true)
        loopIteration();
#else
    emscripten_set_main_loop(&loopIteration, 0, true);
#endif

    return nullptr;
}

} // namespace

void createRenderThread()
{
    auto f = threadInitialized.get_future();
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    emscripten_pthread_attr_settransferredcanvases(&attr, "display");
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    int rc = pthread_create(&thread, &attr, &renderThreadEntry, nullptr);
    assert(rc == 0);
    f.get();
}

vec3 getWorldPosition(const vec3 &input)
{
    std::lock_guard<std::mutex> lock(mutex);
    vec3 wp;
    view->getWorldPosition(input.data(), wp.data());
    return wp;
}

