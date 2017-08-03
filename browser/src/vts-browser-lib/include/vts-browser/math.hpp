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

#ifndef MATH_H_wegfzebvgfhjusd
#define MATH_H_wegfzebvgfhjusd

#include <Eigen/Dense>

#include "foundation.hpp"

namespace vts
{

typedef Eigen::Vector2f vec2f;
typedef Eigen::Vector3f vec3f;
typedef Eigen::Vector4f vec4f;
typedef Eigen::Matrix2f mat2f;
typedef Eigen::Matrix3f mat3f;
typedef Eigen::Matrix4f mat4f;

typedef Eigen::Vector2d vec2;
typedef Eigen::Vector3d vec3;
typedef Eigen::Vector4d vec4;
typedef Eigen::Matrix2d mat2;
typedef Eigen::Matrix3d mat3;
typedef Eigen::Matrix4d mat4;

VTS_API vec3 cross(const vec3 &a, const vec3 &b);
VTS_API double dot(const vec3 &a, const vec3 &b);
VTS_API double dot(const vec2 &a, const vec2 &b);
VTS_API double length(const vec3 &a);
VTS_API double length(const vec2 &a);
VTS_API vec3 normalize(const vec3 &a);

VTS_API mat4 frustumMatrix(double left, double right,
                           double bottom, double top,
                           double near, double far);
VTS_API mat4 perspectiveMatrix(double fovyDegs, double aspect,
                               double near, double far);
VTS_API mat4 lookAt(const vec3 &eye, const vec3 &target, const vec3 &up);
VTS_API mat4 lookAt(const vec3 &eye, const vec3 &target);
VTS_API mat3 upperLeftSubMatrix(const mat4 &mat);

VTS_API mat4 identityMatrix();
VTS_API mat4 rotationMatrix(int axis, double degrees);
VTS_API mat4 scaleMatrix(double sx, double sy, double sz);
VTS_API mat4 scaleMatrix(double s);
VTS_API mat4 translationMatrix(double tx, double ty, double tz);
VTS_API mat4 translationMatrix(const vec3 &vec);

VTS_API vec3 min(const vec3 &a, const vec3 &b);
VTS_API vec3 max(const vec3 &a, const vec3 &b);

VTS_API vec4 vec3to4(vec3 v, double w);
VTS_API vec3 vec4to3(vec4 v, bool division = false);
VTS_API vec3 vec2to3(vec2 v, double w);
VTS_API vec2 vec3to2(vec3 v, bool division = false);

VTS_API vec4f vec3to4f(vec3f v, float w);
VTS_API vec3f vec4to3f(vec4f v, bool division = false);
VTS_API vec3f vec2to3f(vec2f v, float w);
VTS_API vec2f vec3to2f(vec3f v, bool division = false);

VTS_API vec3 anyPerpendicular(const vec3 &v);

VTS_API double degToRad(double angle);
VTS_API double radToDeg(double angle);

VTS_API double modulo(double a, double m);
VTS_API double interpolate(double a, double b, double f);
VTS_API double clamp(double a, double min, double max);
VTS_API double smoothstep(double f);
VTS_API double smootherstep(double f);

VTS_API vec3f convertRgbToHsv(const vec3f &inColor);
VTS_API vec3f convertHsvToRgb(const vec3f &inColor);

template<class T, class U>
T vecFromUblas(const U &u)
{
    T res;
    for (unsigned i = 0; i < u.size(); i++)
        res(i) = u[i];
    return res;
}

template<class U, class T>
U vecToUblas(const T &t)
{
    U res;
    for (unsigned i = 0; i < res.size(); i++)
        res[i] = t(i);
    return res;
}

VTS_API void normalizeAngle(double &a);
VTS_API double angularDiff(double a, double b);
VTS_API vec3 angularDiff(const vec3 &a, const vec3 &b);

VTS_API double aabbPointDist(const vec3 &point,
                             const vec3 &min, const vec3 &max);

} // namespace vts

#endif // MATH_H_wegfzebvgfhjusd
