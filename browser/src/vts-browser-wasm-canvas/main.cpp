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
#include <vts-browser/math.hpp>
#include <vts-browser/map.hpp>
#include <vts-browser/mapOptions.hpp>
#include <vts-browser/mapStatistics.hpp>
#include <vts-browser/camera.hpp>
#include <vts-browser/cameraOptions.hpp>
#include <vts-browser/cameraStatistics.hpp>
#include <vts-browser/navigation.hpp>
#include <vts-browser/navigationOptions.hpp>
#include <vts-browser/position.hpp>
#include <vts-renderer/renderer.hpp>

#include <sstream>
#include <iomanip>
#include <chrono>

#include <emscripten.h>
#include <emscripten/html5.h>

using vts::vec3;
using timerClock = std::chrono::high_resolution_clock;
using timerPoint = std::chrono::time_point<timerClock>;

std::string jsonToHtml(const std::string &json);
std::string positionToHtml(const vts::Position &pos);

std::shared_ptr<vts::Map> map;
std::shared_ptr<vts::Camera> cam;
std::shared_ptr<vts::Navigation> nav;
std::shared_ptr<vts::renderer::RenderContext> context;
std::shared_ptr<vts::renderer::RenderView> view;
vec3 prevMousePosition;
timerPoint lastFrameTimestamp;
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;

timerPoint now()
{
    return timerClock::now();
}

EM_BOOL mouseEvent(int eventType, const EmscriptenMouseEvent *e, void *)
{
    vec3 current = vec3(e->clientX, e->clientY, 0);
    vec3 move = current - prevMousePosition;
    prevMousePosition = current;
    if (!map->getMapconfigAvailable())
        return false;
    switch (eventType)
    {
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
        switch (e->buttons)
        {
        case 1: // LMB
            nav->pan(move.data());
            break;
        case 2: // RMB
            nav->rotate(move.data());
            break;
        }
        break;
    case EMSCRIPTEN_EVENT_DBLCLICK:
        if (e->button == 0) // LMB
        {
            vec3 wp;
            view->getWorldPosition(current.data(), wp.data());
            if (!std::isnan(wp[0]) && !std::isnan(wp[1]) && !std::isnan(wp[2]))
            {
                vec3 np;
                map->convert(wp.data(), np.data(),
                             vts::Srs::Physical, vts::Srs::Navigation);
                nav->setPoint(np.data());
                nav->options().type = vts::NavigationType::Quick;
            }
        }
        break;
    }
    return true;
}

EM_BOOL wheelEvent(int, const EmscriptenWheelEvent *e, void *)
{
    double d = e->deltaY;
    switch (e->deltaMode)
    {
    case DOM_DELTA_PIXEL:
        d /= 30;
        break;
    case DOM_DELTA_LINE:
        break;
    case DOM_DELTA_PAGE:
        d *= 80;
        break;
    }
    nav->zoom(d * -0.21);
    return true;
}

void updateResolution()
{
    int w = 0, h = 0;
    emscripten_webgl_get_drawing_buffer_size(ctx, &w, &h);
    auto &ro = view->options();
    ro.width = w;
    ro.height = h;
    cam->setViewportSize(ro.width, ro.height);
}

EM_JS(void, setHtml,
      (const char *id, const char *value),
{
    document.getElementById(UTF8ToString(id)).innerHTML = UTF8ToString(value)
});

std::string toTime(const timerPoint &s, const timerPoint &e)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << std::chrono::duration_cast<std::chrono::microseconds>(
                    e - s).count() / 1e6 << " s";
    return ss.str();
}

void loopIteration()
{
    timerPoint currentTimestamp = now();
    updateResolution();
    timerPoint a = now();
    map->dataUpdate();
    timerPoint b = now();
    {
        double elapsedTime = std::chrono::duration_cast<
                std::chrono::microseconds>(
                currentTimestamp - lastFrameTimestamp).count() / 1e6;
        map->renderUpdate(elapsedTime);
    }
    timerPoint c = now();
    cam->renderUpdate();
    timerPoint d = now();
    view->render();
    timerPoint e = now();
    if ((map->statistics().renderTicks % 10) == 0)
    {
        setHtml("timeFrame",
                toTime(lastFrameTimestamp, currentTimestamp).c_str());
        setHtml("timeData", toTime(a, b).c_str());
        setHtml("timeMap", toTime(b, c).c_str());
        setHtml("timeCamera", toTime(c, d).c_str());
        setHtml("timeView", toTime(d, e).c_str());
        setHtml("statisticsMap",
                jsonToHtml(map->statistics().toJson()).c_str());
        setHtml("statisticsCamera",
                jsonToHtml(cam->statistics().toJson()).c_str());
        setHtml("position",
                positionToHtml(nav->getPosition()).c_str());
    }
    lastFrameTimestamp = currentTimestamp;
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

    // initialize event callbacks
    emscripten_set_mouseenter_callback("#canvas", nullptr, true, &mouseEvent);
    emscripten_set_mousedown_callback("#canvas", nullptr, true, &mouseEvent);
    emscripten_set_mousemove_callback("#canvas", nullptr, true, &mouseEvent);
    emscripten_set_dblclick_callback("#canvas", nullptr, true, &mouseEvent);
    emscripten_set_wheel_callback("#canvas", nullptr, true, &wheelEvent);

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
    lastFrameTimestamp = now();
    emscripten_set_main_loop(&loopIteration, 0, true);
    return 0;
}


