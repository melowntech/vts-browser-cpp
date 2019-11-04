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

#ifndef SHAPES_HPP_sdrz5tui7u89ze8414
#define SHAPES_HPP_sdrz5tui7u89ze8414

#include <vts-browser/math.hpp>

namespace vts { namespace renderer
{

struct Rect
{
    vec2f a, b;
    Rect();
    Rect(const vec2f &a, const vec2f &b);
    bool valid() const;
    float width() const;
    float height() const;
};

struct Circle
{
    vec2f p;
    float r;
    Circle();
    Circle(const vec2f &p, float r);
    bool valid() const;
};

Rect merge(const Rect &a, const Rect &b);
Rect c2r(const Circle &c);
Circle r2c(const Rect &r);

bool overlaps(const Rect &a, const Rect &b);
bool overlaps(const Circle &a, const Rect &b);
bool overlaps(const Circle &a, const Circle &b);
bool overlaps(const Rect &a, const Circle &b);

double raySphereTest(const vec3 &orig, const vec3 &dir, double radius);
double rayEllipsoidTest(const vec3 &orig, const vec3 &dir,
    double radiusXY, double radiusZ);

} } // namespace vts renderer

#endif
