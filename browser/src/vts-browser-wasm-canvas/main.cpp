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

pthread_t main_thread_id;

DurationBuffer durationMainFrame, durationMainMap, durationMainCamera;

namespace
{

TimerPoint lastFrameTimestamp;

void updateStatisticsHtml()
{
    // timing
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << "<table>";
        ss << "<tr><td>main frame<td class=number><b>"
           << durationMainFrame.avg()
           << "</b><td class=number>"
           << durationMainFrame.max() << "</tr>";
        ss << "<tr><td>main map<td class=number>"
           << durationMainMap.avg()
           << "<td class=number>"
           << durationMainMap.max() << "</tr>";
        ss << "<tr><td>main camera<td class=number>"
           << durationMainCamera.avg()
           << "<td class=number>"
           << durationMainCamera.max() << "</tr>";
        ss << "<tr><td>render frame<td class=number><b>"
           << durationRenderFrame.avg()
           << "</b><td class=number>"
           << durationRenderFrame.max() << "</tr>";
        ss << "<tr><td>render data<td class=number>"
           << durationRenderData.avg()
           << "<td class=number>"
           << durationRenderData.max() << "</tr>";
        ss << "<tr><td>render render<td class=number>"
           << durationRenderRender.avg()
           << "<td class=number>"
           << durationRenderRender.max() << "</tr>";
        ss << "<tr><td>render swap<td class=number>"
           << durationRenderSwap.avg()
           << "<td class=number>"
           << durationRenderSwap.max() << "</tr>";
        ss << "</table>";
        setHtml("statisticsTiming", ss.str());
    }

    // statistics
    setHtml("statisticsMap",
            jsonToHtml(map->statistics().toJson()));
    setHtml("statisticsCamera",
            jsonToHtml(cam->statistics().toJson()));

    // position
    vts::Position pos = nav->getPosition();
    setInputValue("positionCurrent", pos.toUrl());
    setHtml("positionTable", positionToHtml(pos));

    // credits
    setHtml("credits", cam->credits().textShort());
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
    setHtml("viewPreset", ss.str());
}

void updateSearch()
{
    if (srch && srch->done)
    {
        std::stringstream ss;
        for (const auto &it : srch->results)
        {
            ss << "<div class=searchItem><hr>";
            ss << it.title;
            ss << "<button "
               << "onclick=\"gotoPosition("
               << it.position[0] << ", "
               << it.position[1] << ", "
               << it.position[2] << ", "
               << it.radius << ")\">Go</button>";
            ss << "</div>";
        }
        setHtml("searchResults", ss.str());
        srch.reset();
    }
}

void loopIteration()
{
    vts::log(vts::LogLevel::info2, "Main loop iteration");

    updateSearch();

    TimerPoint a = now();
    {
        TimerPoint currentTimestamp = now();
        durationMainFrame.update(lastFrameTimestamp, currentTimestamp);
        double elapsedTime = std::chrono::duration_cast<
                std::chrono::microseconds>(
                currentTimestamp - lastFrameTimestamp).count() / 1e6;
        lastFrameTimestamp = currentTimestamp;
        map->renderUpdate(elapsedTime);
    }

    TimerPoint b = now();
    cam->renderUpdate();

    TimerPoint c = now();
    {
        std::unique_ptr<vts::renderer::RenderDraws> rd;
        drawsQueue2.waitPop(rd);
        rd->swap(cam.get());
        drawsQueue.push(std::move(rd));
    }

    durationMainMap.update(a, b);
    durationMainCamera.update(b, c);
    if ((map->statistics().renderTicks % DurationBuffer::N) == 0)
        updateStatisticsHtml();
}

} // namespace

int main(int, char *[])
{
    //vts::setLogMask(vts::LogLevel::verbose);

    main_thread_id = pthread_self();

    // initialize browser and renderer
    vts::log(vts::LogLevel::info3, "Creating vts browser");
    map = std::make_shared<vts::Map>();
    cam = map->createCamera();
    nav = cam->createNavigation();
    map->callbacks().mapconfigAvailable = &mapconfAvailable;

    map->setMapconfigPath("https://cdn.melown.com/mario/store/melown2015/"
            "map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json");

    {
        auto &m = map->options();
        m.targetResourcesMemoryKB = 512 * 1024;
        auto &c = cam->options();
    }

    // initialize event callbacks
    emscripten_set_mouseenter_callback("#display", nullptr, true, &mouseEvent);
    emscripten_set_mousedown_callback("#display", nullptr, true, &mouseEvent);
    emscripten_set_mousemove_callback("#display", nullptr, true, &mouseEvent);
    emscripten_set_dblclick_callback("#display", nullptr, true, &mouseEvent);
    emscripten_set_wheel_callback("#display", nullptr, true, &wheelEvent);
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW
                                   , nullptr, true, &resizeEvent);
    resizeEvent(0, nullptr, nullptr);

    // start rendering thread
    vts::log(vts::LogLevel::info3, "Create rendering thread");
    createRenderThread();

    // callback into javascript
    vts::log(vts::LogLevel::info3, "Notify JS that the map is ready");
    MAIN_THREAD_ASYNC_EM_ASM(
        Module.onMapCreated()
    );

    // run the game loop
    vts::log(vts::LogLevel::info3, "Starting main loop");
    lastFrameTimestamp = now();
    emscripten_set_main_loop(&loopIteration, 0, true);
    return 0;
}


