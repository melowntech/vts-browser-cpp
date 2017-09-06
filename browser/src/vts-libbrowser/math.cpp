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

#include <dbglog/dbglog.hpp>

#include "include/vts-browser/map.hpp"
#include "include/vts-browser/math.hpp"

namespace vts
{

namespace
{

double &at(mat3 &a, uint32 i)
{
    return a(i % 3, i / 3);
}

double &at(mat4 &a, uint32 i)
{
    return a(i % 4, i / 4);
}

float &at(mat3f &a, uint32 i)
{
    return a(i % 3, i / 3);
}

float &at(mat4f &a, uint32 i)
{
    return a(i % 4, i / 4);
}

double atc(const mat3 &a, uint32 i)
{
    return a(i % 3, i / 3);
}

double atc(const mat4 &a, uint32 i)
{
    return a(i % 4, i / 4);
}

float atc(const mat3f &a, uint32 i)
{
    return a(i % 3, i / 3);
}

float atc(const mat4f &a, uint32 i)
{
    return a(i % 4, i / 4);
}

} // namespace

vec3 cross(const vec3 &a, const vec3 &b)
{
    return vec3(
        a(1) * b(2) - a(2) * b(1),
        a(2) * b(0) - a(0) * b(2),
        a(0) * b(1) - a(1) * b(0)
    );
}

double dot(const vec3 &a, const vec3 &b)
{
    return a(0) * b(0) + a(1) * b(1) + a(2) * b(2);
}

double dot(const vec2 &a, const vec2 &b)
{
    return a(0) * b(0) + a(1) * b(1);
}

double length(const vec3 &a)
{
    return sqrt(dot(a, a));
}

double length(const vec2 &a)
{
    return sqrt(dot(a, a));
}

vec3 normalize(const vec3 &a)
{
    return a / length(a);
}

vec3 anyPerpendicular(const vec3 &v)
{
    vec3 b = normalize(v);
    vec3 a = std::abs(dot(b, vec3(0, 0, 1))) > 0.9
                            ? vec3(0,1,0) : vec3(0,0,1);
    return cross(b, a);
}

vec3 min(const vec3 &a, const vec3 &b)
{
    return vec3(std::min(a(0), b(0)),
                std::min(a(1), b(1)),
                std::min(a(2), b(2)));
}

vec3 max(const vec3 &a, const vec3 &b)
{
    return vec3(std::max(a(0), b(0)),
                std::max(a(1), b(1)),
                std::max(a(2), b(2)));
}

mat4 frustumMatrix(double left, double right,
                   double bottom, double top,
                   double near, double far)
{
    double w(right - left);
    double h(top - bottom);
    double d(far - near);

    return (mat4() <<
            2*near/w, 0, (right+left)/w, 0,
            0, 2*near/h, (top+bottom)/h, 0,
            0, 0, -(far+near)/d, -2*far*near/d,
            0, 0, -1, 0).finished();
}

mat4 perspectiveMatrix(double fovyDegs, double aspect,
                             double near, double far)
{
    double ymax = near * tanf(fovyDegs * M_PI / 360.0);
    double xmax = ymax * aspect;
    return frustumMatrix(-xmax, xmax, -ymax, ymax, near, far);
}

mat4 lookAt(const vec3 &eye, const vec3 &target, const vec3 &up)
{
    vec3 f = normalize(target - eye);
    vec3 u = normalize(up);
    vec3 s = normalize(cross(f, u));
    u = cross(s, f);
    mat4 res;
    at(res, 0) = s(0);
    at(res, 4) = s(1);
    at(res, 8) = s(2);
    at(res, 1) = u(0);
    at(res, 5) = u(1);
    at(res, 9) = u(2);
    at(res, 2) = -f(0);
    at(res, 6) = -f(1);
    at(res, 10) = -f(2);
    at(res, 12) = -dot(s, eye);
    at(res, 13) = -dot(u, eye);
    at(res, 14) = dot(f, eye);
    at(res, 3) = 0;
    at(res, 7) = 0;
    at(res, 11) = 0;
    at(res, 15) = 1;
    return res;
}

mat4 lookAt(const vec3 &a, const vec3 &b)
{
    vec3 d = b - a;
    vec3 u = anyPerpendicular(d);
    return lookAt(a, b, u).inverse() * scaleMatrix(length(d));
}

mat4 identityMatrix4()
{
    return (mat4() <<
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1).finished();
}

mat3 identityMatrix3()
{
    return (mat3() <<
            1, 0, 0,
            0, 1, 0,
            0, 0, 1).finished();
}

mat4 rotationMatrix(int axis, double degrees)
{
    double radians = degToRad(degrees);
    double ca(cos(radians)), sa(sin(radians));

    switch (axis)
    {
    case 0:
        return (mat4() <<
                1,  0,  0, 0,
                0, ca,-sa, 0,
                0, sa, ca, 0,
                0,  0,  0, 1).finished();
    case 1:
        return (mat4() <<
                ca, 0,-sa, 0,
                0, 1,  0, 0,
                sa, 0, ca, 0,
                0, 0,  0, 1).finished();
    case 2:
        return (mat4() <<
                ca,-sa, 0, 0,
                sa, ca, 0, 0,
                0,  0,  1, 0,
                0,  0,  0, 1).finished();
    default:
        LOGTHROW(fatal, std::invalid_argument)
                << "Invalid rotation axis index";
    }
    throw; // shut up compiler warning
}

mat4 scaleMatrix(double sx, double sy, double sz)
{
    return (mat4() <<
            sx,  0,  0, 0,
            0, sy,  0, 0,
            0,  0, sz, 0,
            0,  0,  0, 1).finished();
}

mat4 scaleMatrix(double s)
{
    return scaleMatrix(s, s, s);
}

mat4 translationMatrix(double tx, double ty, double tz)
{
    return (mat4() <<
            1, 0, 0, tx,
            0, 1, 0, ty,
            0, 0, 1, tz,
            0, 0, 0, 1).finished();
}

mat4 translationMatrix(const vec3 &vec)
{
    return translationMatrix(vec(0), vec(1), vec(2));
}

vec4 vec3to4(vec3 v, double w)
{
    vec4 res;
    res(0) = v(0);
    res(1) = v(1);
    res(2) = v(2);
    res(3) = w;
    return res;
}

vec3 vec4to3(vec4 v, bool division)
{
    vec3 res;
    if (division)
        v = v / v(3);
    res(0) = v(0);
    res(1) = v(1);
    res(2) = v(2);
    return res;
}

vec3 vec2to3(vec2 v, double w)
{
    vec3 res;
    res(0) = v(0);
    res(1) = v(1);
    res(2) = w;
    return res;
}

vec2 vec3to2(vec3 v, bool division)
{
    vec2 res;
    if (division)
        v = v / v(2);
    res(0) = v(0);
    res(1) = v(1);
    return res;
}

vec4f vec3to4f(vec3f v, float w)
{
    vec4f res;
    res(0) = v(0);
    res(1) = v(1);
    res(2) = v(2);
    res(3) = w;
    return res;
}

vec3f vec4to3f(vec4f v, bool division)
{
    vec3f res;
    if (division)
        v = v / v(3);
    res(0) = v(0);
    res(1) = v(1);
    res(2) = v(2);
    return res;
}

vec3f vec2to3f(vec2f v, float w)
{
    vec3f res;
    res(0) = v(0);
    res(1) = v(1);
    res(2) = w;
    return res;
}

vec2f vec3to2f(vec3f v, bool division)
{
    vec2f res;
    if (division)
        v = v / v(2);
    res(0) = v(0);
    res(1) = v(1);
    return res;
}

mat3 mat4to3(const mat4 &mat)
{
    mat3 res;
    at(res, 0) = atc(mat, 0);
    at(res, 1) = atc(mat, 1);
    at(res, 2) = atc(mat, 2);
    at(res, 3) = atc(mat, 4);
    at(res, 4) = atc(mat, 5);
    at(res, 5) = atc(mat, 6);
    at(res, 6) = atc(mat, 8);
    at(res, 7) = atc(mat, 9);
    at(res, 8) = atc(mat, 10);
    return res;
}

mat4 mat3to4(const mat3 &mat)
{
    mat4 res;
    at(res, 0) = atc(mat, 0);
    at(res, 1) = atc(mat, 1);
    at(res, 2) = atc(mat, 2);
    at(res, 3) = 0;
    at(res, 4) = atc(mat, 3);
    at(res, 5) = atc(mat, 4);
    at(res, 6) = atc(mat, 5);
    at(res, 7) = 0;
    at(res, 8) = atc(mat, 6);
    at(res, 9) = atc(mat, 7);
    at(res, 10) = atc(mat, 8);
    at(res, 11) = 0;
    at(res, 12) = 0;
    at(res, 13) = 0;
    at(res, 14) = 0;
    at(res, 15) = 1;
    return res;
}

double modulo(double a, double m)
{
    a = std::fmod(a, m);
    if (a < 0)
        a += m;
    if (!(a < m))
        a = 0;
    assert(a >= 0 && a < m);
    return a;
}

double clamp(double a, double min, double max)
{
    assert(min <= max);
    return std::max(std::min(a, max), min);
}

double interpolate(double a, double b, double f)
{
    return (b - a) * f + a;
}

double smoothstep(double f)
{
    return f * f * (3 - f * 2);
}

double smootherstep(double f)
{
    return f * f * f * (f * (f * 6 - 15) + 10);
}

void normalizeAngle(double &a)
{
    a = modulo(a, 360);
}

double degToRad(double angle)
{
    return angle * M_PI / 180;
}

double radToDeg(double angle)
{
    return angle * 180 / M_PI;
}

double angularDiff(double a, double b)
{
    normalizeAngle(a);
    normalizeAngle(b);
    double c = b - a;
    if (c > 180)
        c = c - 360;
    else if (c < -180)
        c = c + 360;
    assert(c >= -180 && c <= 180);
    return c;
}

vec3 angularDiff(const vec3 &a, const vec3 &b)
{
    return vec3(
                angularDiff(a(0), b(0)),
                angularDiff(a(1), b(1)),
                angularDiff(a(2), b(2))
                );
}

double aabbPointDist(const vec3 &point, const vec3 &min, const vec3 &max)
{
    double r = 0;
    for (int i = 0; i < 3; i++)
        r += std::max(std::max(min[i] - point[i], point[i] - max[i]), 0.0);
    return sqrt(r);
}

vec3 rawToVec3(const double v[3])
{
    return vec3(v[0], v[1], v[2]);
}

vec4 rawToVec4(const double v[4])
{
    return vec4(v[0], v[1], v[2], v[3]);
}

mat3 rawToMat3(const double v[9])
{
    mat3 r;
    for (int i = 0; i < 9; i++)
        at(r, i) = v[i];
    return r;
}

mat4 rawToMat4(const double v[16])
{
    mat4 r;
    for (int i = 0; i < 16; i++)
        at(r, i) = v[i];
    return r;
}

vec3f rawToVec3(const float v[3])
{
    return vec3f(v[0], v[1], v[2]);
}

vec4f rawToVec4(const float v[4])
{
    return vec4f(v[0], v[1], v[2], v[3]);
}

mat3f rawToMat3(const float v[9])
{
    mat3f r;
    for (int i = 0; i < 9; i++)
        at(r, i) = v[i];
    return r;
}

mat4f rawToMat4(const float v[16])
{
    mat4f r;
    for (int i = 0; i < 16; i++)
        at(r, i) = v[i];
    return r;
}

void vecToRaw(const vec3 &a, double v[3])
{
    for (int i = 0; i < 3; i++)
        v[i] = a[i];
}

void vecToRaw(const vec4 &a, double v[4])
{
    for (int i = 0; i < 4; i++)
        v[i] = a[i];
}

void matToRaw(const mat3 &a, double v[9])
{
    for (int i = 0; i < 9; i++)
        v[i] = atc(a, i);
}

void matToRaw(const mat4 &a, double v[16])
{
    for (int i = 0; i < 16; i++)
        v[i] = atc(a, i);
}

void vecToRaw(const vec3f &a, float v[3])
{
    for (int i = 0; i < 3; i++)
        v[i] = a[i];
}

void vecToRaw(const vec4f &a, float v[4])
{
    for (int i = 0; i < 4; i++)
        v[i] = a[i];
}

void matToRaw(const mat3f &a, float v[9])
{
    for (int i = 0; i < 9; i++)
        v[i] = atc(a, i);
}

void matToRaw(const mat4f &a, float v[16])
{
    for (int i = 0; i < 16; i++)
        v[i] = atc(a, i);
}


} // namespace vts

