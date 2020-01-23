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

#include <vts-browser/navigationOptions.hpp>

#include <emscripten/html5.h>

vec3 getWorldPosition(const vec3 &input);

vec3 prevMousePosition;

EM_BOOL mouseEvent(int eventType, const EmscriptenMouseEvent *e, void *)
{
    //vts::log(vts::LogLevel::info2, "Mouse event");
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
            vec3 wp = getWorldPosition(current);
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
    //vts::log(vts::LogLevel::info2, "Mouse wheel event");
    if (!map || !map->getMapconfigAvailable())
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
    nav->zoom(d * -0.25);
    nav->options().type = vts::NavigationType::Quick;
    return true;
}

