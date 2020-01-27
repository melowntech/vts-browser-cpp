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

#include "vts-libbrowser/utilities/json.hpp"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/threading.h>

#include <cstdlib> // std::malloc
#include <cstring> // std::strcpy

// CPP -> JS

extern "C" void setHtmlImpl(const char *id, char *str)
{
    EM_ASM({
        document.getElementById(UTF8ToString($0)).innerHTML = UTF8ToString($1)
    }, id, str);
    std::free(str);
}

extern "C" void setInputValueImpl(const char *id, char *str)
{
    EM_ASM({
        document.getElementById(UTF8ToString($0)).value = UTF8ToString($1)
    }, id, str);
    std::free(str);
}

void setHtml(const char *id, const std::string &value)
{
    char *str = (char*)std::malloc(value.length() + 1);
    std::strcpy(str, value.c_str());
    emscripten_async_run_in_main_runtime_thread(
                EM_FUNC_SIG_VII, &setHtmlImpl, id, str);
}

void setInputValue(const char *id, const std::string &value)
{
    char *str = (char*)std::malloc(value.length() + 1);
    std::strcpy(str, value.c_str());
    emscripten_async_run_in_main_runtime_thread(
                EM_FUNC_SIG_VII, &setInputValueImpl, id, str);
}

// JS -> CPP

extern "C" EMSCRIPTEN_KEEPALIVE void setMapconfig(const char *url)
{
    if(!map)
        return;
    map->setMapconfigPath(url);
}

extern "C" EMSCRIPTEN_KEEPALIVE void setViewPreset(const char *preset)
{
    if(!map || !map->getMapconfigAvailable())
        return;
    map->setViewCurrent(preset);
}

extern "C" EMSCRIPTEN_KEEPALIVE void setPosition(const char *pos)
{
    if(!nav || !map->getMapconfigAvailable())
        return;
    try
    {
        vts::Position p(pos);
        nav->setPosition(p);
    }
    catch (...)
    {
        // do nothing
    }
}

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
    if(!map)
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
    if(!map)
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

