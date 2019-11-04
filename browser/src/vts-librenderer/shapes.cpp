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

#include "shapes.hpp"

namespace vts { namespace renderer
{

Rect::Rect() : a(nan2().cast<float>()), b(nan2().cast<float>())
{}

Rect::Rect(const vec2f &a, const vec2f &b) : a(a), b(b)
{
    assert(a[0] <= b[0]);
    assert(a[1] <= b[1]);
}

bool Rect::valid() const
{
    return !std::isnan(a[0]) && !std::isnan(a[1])
        && !std::isnan(b[0]) && !std::isnan(b[1]);
}

float Rect::width() const
{
    return b[0] - a[0];
}

float Rect::height() const
{
    return b[1] - a[1];
}

Circle::Circle() : p(nan2().cast<float>()), r((float)nan1())
{}

Circle::Circle(const vec2f &p, float r) : p(p), r(r)
{}

bool Circle::valid() const
{
    return !std::isnan(p[0]) && !std::isnan(p[1]) && r >= 0;
}

Rect merge(const Rect &a, const Rect &b)
{
    if (!a.valid())
        return b;
    if (!b.valid())
        return a;
    Rect r;
    for (int i = 0; i < 2; i++)
    {
        r.a[i] = std::min(a.a[i], b.a[i]);
        r.b[i] = std::max(a.b[i], b.b[i]);
    }
    return r;
}

Rect c2r(const Circle &a)
{
    const vec2f s(a.r, a.r);
    return Rect(a.p - s, a.p + s);
}

Circle r2c(const Rect &r)
{
    return Circle((r.a + r.b) * 0.5f,
        std::min(r.width(), r.height()) * 0.5f);
}

bool overlaps(const Rect &a, const Rect &b)
{
    for (int i = 0; i < 2; i++)
    {
        if (a.b[i] < b.a[i])
            return false;
        if (a.a[i] > b.b[i])
            return false;
    }
    return true;
}

namespace
{
    float sqr(float a)
    {
        return a * a;
    }

    float lenSqr(const vec2f &v)
    {
        return dot(v, v);
    }

    float distSqr(const vec2f &a, const vec2f &b)
    {
        return lenSqr(vec2f(a - b));
    }
}

bool overlaps(const Circle &a, const Rect &b)
{
    vec2f closest = vec2f(
        clamp(a.p[0], b.a[0], b.b[0]),
        clamp(a.p[1], b.a[1], b.b[1]));
    return distSqr(a.p, closest) <= sqr(a.r);
}

bool overlaps(const Circle &a, const Circle &b)
{
    return distSqr(a.p, b.p) <= sqr(a.r + b.r);
}

bool overlaps(const Rect &a, const Circle &b)
{
    return overlaps(b, a);
}

double raySphereTest(const vec3 &orig, const vec3 &dir, double radius)
{
    double radius2 = radius * radius;
    vec3 L = -orig;
    double tca = dot(L, dir);
    double d2 = dot(L, L) - tca * tca;
    if (d2 > radius2)
        return nan1();
    double thc = std::sqrt(radius2 - d2);
    double t0 = tca - thc;
    double t1 = tca + thc;
    if (t0 > t1)
        std::swap(t0, t1);
    if (t0 < 0)
    {
        t0 = t1;
        if (t0 < 0)
            return nan1();
    }
    return t0;
}

double rayEllipsoidTest(const vec3 &orig, const vec3 &dir,
    double radiusXY, double radiusZ)
{
    double r = radiusXY / radiusZ;
    vec3 o = vec3(orig[0], orig[1], orig[2] * r); // ellipsoid to sphere
    vec3 d = vec3(dir[0], dir[1], dir[2] * r);
    d = normalize(d);
    double t = raySphereTest(o, d, radiusXY);
    if (std::isnan(t))
        return nan1();
    vec3 p = o + d * t;
    p = vec3(p[0], p[1], p[2] / r); // sphere to ellipsoid
    return length(vec3(p - o));
}

} } // namespace vts renderer

