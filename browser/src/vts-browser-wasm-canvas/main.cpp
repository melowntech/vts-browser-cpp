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

#include "common.hpp"
#include "timer.hpp"

#include <vts-browser/mapOptions.hpp>
#include <vts-browser/mapCallbacks.hpp>
#include <vts-browser/mapStatistics.hpp>
#include <vts-browser/cameraStatistics.hpp>
#include <vts-browser/cameraCredits.hpp>

#include <iomanip>

std::shared_ptr<vts::Map> map;
std::shared_ptr<vts::Camera> cam;
std::shared_ptr<vts::Navigation> nav;
std::shared_ptr<vts::SearchTask> srch;
std::shared_ptr<vts::renderer::RenderContext> context;
std::shared_ptr<vts::renderer::RenderView> view;
TimerPoint lastFrameTimestamp;
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;

void updateResolution()
{
    int w = 0, h = 0;
    emscripten_webgl_get_drawing_buffer_size(ctx, &w, &h);
    auto &ro = view->options();
    ro.width = w;
    ro.height = h;
    cam->setViewportSize(ro.width, ro.height);
}

DurationBuffer durationFrame, durationData,
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

    // statistics
    setHtml("statisticsMap",
            jsonToHtml(map->statistics().toJson()).c_str());
    setHtml("statisticsCamera",
            jsonToHtml(cam->statistics().toJson()).c_str());

    // position
    vts::Position pos = nav->getPosition();
    setInputValue("positionCurrent", pos.toUrl().c_str());
    setHtml("positionTable", positionToHtml(pos).c_str());

    // credits
    setHtml("credits", cam->credits().textShort().c_str());
}

void mapconfAvailable()
{
    std::stringstream ss;
    std::string current = map->getViewCurrent();
    for (const std::string &s : map->getViewNames())
    {
        ss << "<option";
        if (s == current)
            ss << " selected";
        ss << ">" << s << "</option>\n";
    }
    setHtml("viewPreset", ss.str().c_str());
}

void loopIteration()
{
    TimerPoint currentTimestamp = now();
    updateResolution();

    // search
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

    TimerPoint a = now();
    map->dataUpdate();

    TimerPoint b = now();
    {
        double elapsedTime = std::chrono::duration_cast<
                std::chrono::microseconds>(
                currentTimestamp - lastFrameTimestamp).count() / 1e6;
        map->renderUpdate(elapsedTime);
    }

    TimerPoint c = now();
    cam->renderUpdate();

    TimerPoint d = now();
    view->render();

    TimerPoint e = now();
    durationFrame.update(lastFrameTimestamp, currentTimestamp);
    durationData.update(a, b);
    durationMap.update(b, c);
    durationCamera.update(c, d);
    durationView.update(d, e);
    if ((map->statistics().renderTicks % DurationBuffer::N) == 0)
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
    map->callbacks().mapconfigAvailable = &mapconfAvailable;
    context = std::make_shared<vts::renderer::RenderContext>();
    context->bindLoadFunctions(map.get());
    cam = map->createCamera();
    nav = cam->createNavigation();
    view = context->createView(cam.get());
    updateResolution();

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


