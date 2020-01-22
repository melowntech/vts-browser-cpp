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

#include <vts-browser/log.hpp>
#include <vts-browser/math.hpp>
#include <vts-browser/map.hpp>
#include <vts-browser/camera.hpp>
#include <vts-browser/navigation.hpp>
#include <vts-browser/position.hpp>
#include <vts-browser/search.hpp>
#include <vts-renderer/renderer.hpp>


#include <emscripten.h>
#include <emscripten/html5.h>


using vts::vec3;


std::string jsonToHtml(const std::string &json);
std::string positionToHtml(const vts::Position &pos);

extern "C" void setHtml(const char *id, const char *value);
extern "C" void setInputValue(const char *id, const char *value);

EM_BOOL mouseEvent(int eventType, const EmscriptenMouseEvent *e, void *);
EM_BOOL wheelEvent(int, const EmscriptenWheelEvent *e, void *);


extern std::shared_ptr<vts::Map> map;
extern std::shared_ptr<vts::Camera> cam;
extern std::shared_ptr<vts::Navigation> nav;
extern std::shared_ptr<vts::SearchTask> srch;
extern std::shared_ptr<vts::renderer::RenderContext> context;
extern std::shared_ptr<vts::renderer::RenderView> view;

