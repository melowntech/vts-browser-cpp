/**
 * Copyright (c) 2020 Melown Technologies SE
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

#include <vts-browser/mapOptions.hpp>
#include <vts-browser/cameraOptions.hpp>
#include <vts-browser/navigationOptions.hpp>

// this is internal header from the VTS browser implementation
//   and should not be used in other applications
#include "vts-libbrowser/utilities/json.hpp"

#include <cstdlib> // std::malloc
#include <cstring> // std::strcpy

namespace
{

struct FreeAtExit : private vts::Immovable
{
    explicit FreeAtExit(const char *ptr = nullptr) : ptr(ptr) {}
    ~FreeAtExit() { std::free((void*)ptr); }
protected:
    const char *ptr;
};

char *copyString(const char *str)
{
    char *res = (char*)std::malloc(std::strlen(str) + 1);
    std::strcpy(res, str);
    return res;
}

char *copyString(const std::string &str)
{
    char *res = (char*)std::malloc(str.length() + 1);
    std::strcpy(res, str.c_str());
    return res;
}

} // namespace

// CPP -> JS

namespace
{

void setHtmlImpl(const char *id, char *str)
{
    FreeAtExit freeAtExit{str};
    EM_ASM({
        document.getElementById(UTF8ToString($0)).innerHTML = UTF8ToString($1)
    }, id, str);
}

void setInputValueImpl(const char *id, char *str)
{
    FreeAtExit freeAtExit{str};
    EM_ASM({
        document.getElementById(UTF8ToString($0)).value = UTF8ToString($1)
    }, id, str);
}

} // namespace

void setHtml(const char *id, const std::string &value)
{
    emscripten_async_run_in_main_runtime_thread(
                EM_FUNC_SIG_VII, &setHtmlImpl, id, copyString(value));
}

void setInputValue(const char *id, const std::string &value)
{
    emscripten_async_run_in_main_runtime_thread(
                EM_FUNC_SIG_VII, &setInputValueImpl, id, copyString(value));
}

// JS -> CPP

namespace
{

void setMapconfigImpl(const char *url)
{
    FreeAtExit freeAtExit{url};
    if (!map)
        return;
    map->setMapconfigPath(url);
}

void setViewPresetImpl(const char *preset)
{
    FreeAtExit freeAtExit{preset};
    if (!map || !map->getMapconfigAvailable())
        return;
    map->setViewCurrent(preset);
}

void setPositionImpl(const char *pos)
{
    FreeAtExit freeAtExit{pos};
    if (!nav || !map->getMapconfigAvailable())
        return;
    try
    {
        vts::Position p(pos);
        nav->options().type = vts::NavigationType::FlyOver;
        nav->setPosition(p);
    }
    catch (...)
    {
        // do nothing
    }
}

void searchImpl(const char *query)
{
    FreeAtExit freeAtExit{query};
    if (!map)
        return;
    if (!map->searchable())
    {
        setHtml("searchResults", "Search not available");
        return;
    }
    srch = map->search(query);
    setHtml("searchResults", "Searching...");
}

void gotoPositionImpl(double values[4])
{
    FreeAtExit freeAtExit{(char*)values};
    if (!nav)
        return;
    nav->setViewExtent(std::max(values[3] * 2, 6667.0));
    nav->setRotation({ 0, 270, 0 });
    nav->resetAltitude();
    nav->resetNavigationMode();
    nav->setPoint(values);
    nav->options().type = vts::NavigationType::FlyOver;
}

} // namespace

extern "C" EMSCRIPTEN_KEEPALIVE void setMapconfig(const char *url)
{
    emscripten_dispatch_to_thread_async(main_thread_id, EM_FUNC_SIG_VI,
        &setMapconfigImpl, nullptr, copyString(url));
}

extern "C" EMSCRIPTEN_KEEPALIVE void setViewPreset(const char *preset)
{
    emscripten_dispatch_to_thread_async(main_thread_id, EM_FUNC_SIG_VI,
        &setViewPresetImpl, nullptr, copyString(preset));
}

extern "C" EMSCRIPTEN_KEEPALIVE void setPosition(const char *pos)
{
    emscripten_dispatch_to_thread_async(main_thread_id, EM_FUNC_SIG_VI,
        &setPositionImpl, nullptr, copyString(pos));
}

extern "C" EMSCRIPTEN_KEEPALIVE void search(const char *query)
{
    emscripten_dispatch_to_thread_async(main_thread_id, EM_FUNC_SIG_VI,
        &searchImpl, nullptr, copyString(query));
}

extern "C" EMSCRIPTEN_KEEPALIVE void gotoPosition(
        double x, double y, double z, double ve)
{
    double *arr = (double*)std::malloc(4 * sizeof(double));
    arr[0] = x;
    arr[1] = y;
    arr[2] = z;
    arr[3] = ve;
    emscripten_dispatch_to_thread_async(main_thread_id, EM_FUNC_SIG_VI,
        &gotoPositionImpl, nullptr, arr);
}

// options are not proxied to the main/renderer threads
//   they are mostly safe to modify from any thread

namespace
{

using namespace vts;

void applyRenderOptions(const std::string &json, renderer::RenderOptions &opt)
{
    struct T : public vtsCRenderOptionsBase
    {
        void apply(const std::string &json)
        {
            Json::Value v = stringToJson(json);
            AJ(textScale, asFloat);
            AJ(antialiasingSamples, asUInt);
            AJ(renderGeodataDebug, asUInt);
            AJ(renderAtmosphere, asBool);
            AJ(renderPolygonEdges, asBool);
            AJ(geodataHysteresis, asBool);
            AJ(debugDepthFeedback, asBool);
        }
    };
    T t = (T&)opt;
    t.apply(json);
    opt = (renderer::RenderOptions&)t;
}

std::string getRenderOptions(const renderer::RenderOptions &opt)
{
    struct T : public vtsCRenderOptionsBase
    {
        std::string get() const
        {
            Json::Value v;
            TJ(textScale, asFloat);
            TJ(antialiasingSamples, asUInt);
            TJ(renderGeodataDebug, asUInt);
            TJ(renderAtmosphere, asBool);
            TJ(renderPolygonEdges, asBool);
            TJ(geodataHysteresis, asBool);
            TJ(debugDepthFeedback, asBool);
            return jsonToString(v);
        }
    };
    const T t = (const T&)opt;
    return t.get();
}

} // namespace

extern std::shared_ptr<vts::renderer::RenderView> view; // only for the options

extern "C" EMSCRIPTEN_KEEPALIVE void applyOptions(const char *json)
{
    if (!map)
        return;
    {
        std::stringstream ss;
        ss << "Changing options: " << json;
        vts::log(vts::LogLevel::info2, ss.str());
    }
    map->options().applyJson(json);
    cam->options().applyJson(json);
    nav->options().applyJson(json);
    applyRenderOptions(json, view->options());
}

extern "C" EMSCRIPTEN_KEEPALIVE const char *getOptions()
{
    if (!map)
        return "";
    // NOT REENTRANT!!, for JS interop only (quick and dirty :D)
    static std::string result;
    result = "";
    result += map->options().toJson();
    result += cam->options().toJson();
    result += nav->options().toJson();
    result += getRenderOptions(view->options());
    return result.c_str();
}

