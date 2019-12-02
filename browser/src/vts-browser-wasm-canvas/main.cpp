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
#include <vts-browser/search.hpp>
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
void applyRenderOptions(const std::string &json,
        vts::renderer::RenderOptions &opt);
std::string getRenderOptions(const vts::renderer::RenderOptions &opt);

std::shared_ptr<vts::Map> map;
std::shared_ptr<vts::Camera> cam;
std::shared_ptr<vts::Navigation> nav;
std::shared_ptr<vts::renderer::RenderContext> context;
std::shared_ptr<vts::renderer::RenderView> view;
std::shared_ptr<vts::SearchTask> srch;
vec3 prevMousePosition;
timerPoint lastFrameTimestamp;
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;

EM_JS(void, setHtml, (const char *id, const char *value),
{
    document.getElementById(UTF8ToString(id)).innerHTML = UTF8ToString(value)
});

extern "C" EMSCRIPTEN_KEEPALIVE void search(const char *query)
{
    if(!map)
        return;
    if (!map->searchable())
    {
        setHtml("searchResults", "Search not available");
        return;
    }
    srch = map->search(query);
    setHtml("searchResults", "Searching...");
}

extern "C" EMSCRIPTEN_KEEPALIVE void gotoPosition(
        double x, double y, double z, double ve)
{
    if(!map)
        return;
    nav->setViewExtent(std::max(ve * 2, 6667.0));
    nav->setRotation({0,270,0});
    nav->resetAltitude();
    nav->resetNavigationMode();
    nav->setPoint({x, y, z});
    nav->options().type = vts::NavigationType::FlyOver;
}

extern "C" EMSCRIPTEN_KEEPALIVE void applyOptions(const char *json)
{
    if(!map)
        return;
    {
        std::stringstream ss;
        ss << "Changing options: " << json;
        vts::log(vts::LogLevel::info3, ss.str());
    }
    map->options().applyJson(json);
    cam->options().applyJson(json);
    nav->options().applyJson(json);
    applyRenderOptions(json, view->options());
}

extern "C" EMSCRIPTEN_KEEPALIVE const char *getOptions()
{
    if(!map)
        return "";
    static std::string result; // NOT REENTRANT!!, for JS interop (quick and dirty :D)
    result = "";
    result += map->options().toJson();
    result += cam->options().toJson();
    result += nav->options().toJson();
    result += getRenderOptions(view->options());
    return result.c_str();
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
            nav->options().type = vts::NavigationType::Quick;
            break;
        case 2: // RMB
            nav->rotate(move.data());
            nav->options().type = vts::NavigationType::Quick;
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
    if (!map->getMapconfigAvailable())
        return false;
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

timerPoint now()
{
    return timerClock::now();
}

struct durationsBuffer
{
    static const uint32 N = 60;
    float buffer[N];

    durationsBuffer()
    {
        for (auto &i : buffer)
            i = 0;
    }

    float avg() const
    {
        float sum = 0;
        for (float i : buffer)
            sum += i;
        return sum / N;
    }

    float max() const
    {
        float m = 0;
        for (float i : buffer)
            m = std::max(m, i);
        return m;
    }

    void update(float t)
    {
        buffer[map->statistics().renderTicks % N] = t;
    }

    void update(const timerPoint &a, const timerPoint &b)
    {
        update(std::chrono::duration_cast<
               std::chrono::microseconds>(b - a).count() / 1000.0);
    }
};

durationsBuffer durationFrame, durationData,
    durationMap, durationCamera, durationView;

void updateStatisticsHtml()
{
    // timing
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << "<table>";
        ss << "<tr><td>frame<td class=number>" << durationFrame.avg()
           << "<td class=number>" << durationFrame.max() << "</tr>";
        ss << "<tr><td>data<td class=number>" << durationData.avg()
           << "<td class=number>" << durationData.max() << "</tr>";
        ss << "<tr><td>map<td class=number>" << durationMap.avg()
           << "<td class=number>" << durationMap.max() << "</tr>";
        ss << "<tr><td>camera<td class=number>" << durationCamera.avg()
           << "<td class=number>" << durationCamera.max() << "</tr>";
        ss << "<tr><td>view<td class=number>" << durationView.avg()
           << "<td class=number>" << durationView.max() << "</tr>";
        ss << "</table>";
        setHtml("statisticsTiming", ss.str().c_str());
    }

    setHtml("statisticsMap",
            jsonToHtml(map->statistics().toJson()).c_str());
    setHtml("statisticsCamera",
            jsonToHtml(cam->statistics().toJson()).c_str());
    setHtml("position",
            positionToHtml(nav->getPosition()).c_str());
}

void loopIteration()
{
    timerPoint currentTimestamp = now();
    updateResolution();

    // search
    {
        if (srch && srch->done)
        {
            std::stringstream ss;
            for (const auto &it : srch->results)
            {
                ss << "<div class=searchItem><hr>";
                ss << it.displayName;
                ss << "<button "
                   << "onclick=\"gotoPosition("
                   << it.position[0] << ", "
                   << it.position[1] << ", "
                   << it.position[2] << ", "
                   << it.radius << ")\">Go</button>";
                ss << "</div>";
            }
            setHtml("searchResults", ss.str().c_str());
            srch.reset();
        }
    }

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
    durationFrame.update(lastFrameTimestamp, currentTimestamp);
    durationData.update(a, b);
    durationMap.update(b, c);
    durationCamera.update(c, d);
    durationView.update(d, e);
    if ((map->statistics().renderTicks % durationsBuffer::N) == 0)
        updateStatisticsHtml();
    lastFrameTimestamp = currentTimestamp;
}

int main(int, char *[])
{
    // create OpenGL context
    vts::log(vts::LogLevel::info3, "Creating OpenGL context");
    {
        EmscriptenWebGLContextAttributes attr;
        emscripten_webgl_init_context_attributes(&attr);
        attr.alpha = attr.depth = attr.stencil = attr.antialias = 0; // we have our own render target
        attr.majorVersion = 2; // WebGL 2.0
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

    {
        auto &m = map->options();
        m.targetResourcesMemoryKB = 512 * 1024;
        auto &c = cam->options();
    }

    // callback into javascript
    EM_ASM(
        Module.onMapCreated()
    );

    // run the game loop
    vts::log(vts::LogLevel::info3, "Starting the game loop");
    lastFrameTimestamp = now();
    emscripten_set_main_loop(&loopIteration, 0, true);
    return 0;
}


