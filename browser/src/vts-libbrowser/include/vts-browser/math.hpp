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

#ifndef MATH_HPP_wegfzebvgfhjusd
#define MATH_HPP_wegfzebvgfhjusd

#include "foundation.hpp"

#include <Eigen/Dense>

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

typedef Eigen::Matrix<sint16, 2, 1> vec2si16;
typedef Eigen::Matrix<uint16, 2, 1> vec2ui16;
typedef Eigen::Matrix<sint16, 3, 1> vec3si16;
typedef Eigen::Matrix<uint16, 3, 1> vec3ui16;
typedef Eigen::Matrix<sint16, 4, 1> vec4si16;
typedef Eigen::Matrix<uint16, 4, 1> vec4ui16;

typedef Eigen::Matrix<sint32, 2, 1> vec2si32;
typedef Eigen::Matrix<uint32, 2, 1> vec2ui32;
typedef Eigen::Matrix<sint32, 3, 1> vec3si32;
typedef Eigen::Matrix<uint32, 3, 1> vec3ui32;
typedef Eigen::Matrix<sint32, 4, 1> vec4si32;
typedef Eigen::Matrix<uint32, 4, 1> vec4ui32;

VTS_API double modulo(double a, double m);
VTS_API double smoothstep(double f);
VTS_API double smootherstep(double f);
VTS_API double degToRad(double angle);
VTS_API double radToDeg(double angle);
VTS_API void normalizeAngle(double &a);
VTS_API double angularDiff(double a, double b);
VTS_API vec3 angularDiff(const vec3 &a, const vec3 &b);

VTS_API vec3 cross(const vec3 &a, const vec3 &b);
VTS_API vec3 anyPerpendicular(const vec3 &v);
VTS_API vec3f cross(const vec3f &a, const vec3f &b);
VTS_API vec3f anyPerpendicular(const vec3f &v);

VTS_API mat3 identityMatrix3();
VTS_API mat4 identityMatrix4();
VTS_API mat4 rotationMatrix(int axis, double degrees);
VTS_API mat4 scaleMatrix(double s);
VTS_API mat4 scaleMatrix(double sx, double sy, double sz);
VTS_API mat4 scaleMatrix(const vec3 &vec);
VTS_API mat4 translationMatrix(double tx, double ty, double tz);
VTS_API mat4 translationMatrix(const vec3 &vec);
VTS_API mat4 lookAt(const vec3 &eye, const vec3 &target, const vec3 &up);
VTS_API mat4 lookAt(const vec3 &eye, const vec3 &target);
VTS_API mat4 frustumMatrix(double left, double right,
                           double bottom, double top,
                           double near_, double far_);
VTS_API mat4 perspectiveMatrix(double fovyDegs, double aspect,
                               double near_, double far_);
VTS_API mat4 orthographicMatrix(double left, double right,
                                double bottom, double top,
                                double near_, double far_);

VTS_API double aabbPointDist(const vec3 &point,
                             const vec3 &min, const vec3 &max);
VTS_API bool aabbTest(const vec3 aabb[2], const vec4 planes[6]);
VTS_API void frustumPlanes(const mat4 &vp, vec4 planes[6]);

VTS_API vec2ui16 vec2to2ui16(const vec2 &v, bool normalized = true);
VTS_API vec2ui16 vec2to2ui16(const vec2f &v, bool normalized = true);
VTS_API mat3 mat4to3(const mat4 &mat);
VTS_API mat4 mat3to4(const mat3 &mat);
VTS_API mat3 rawToMat3(const double v[9]);
VTS_API mat4 rawToMat4(const double v[16]);
VTS_API mat3f rawToMat3(const float v[9]);
VTS_API mat4f rawToMat4(const float v[16]);
VTS_API void matToRaw(const mat3 &a, double v[9]);
VTS_API void matToRaw(const mat4 &a, double v[16]);
VTS_API void matToRaw(const mat3f &a, float v[9]);
VTS_API void matToRaw(const mat4f &a, float v[16]);

VTS_API vec3f convertRgbToHsv(const vec3f &inColor);
VTS_API vec3f convertHsvToRgb(const vec3f &inColor);
VTS_API vec3f convertToRainbowColor(float inValue);

inline double clamp(double a, double min, double max)
{
    assert(min <= max);
    return std::max(std::min(a, max), min);
}

inline double interpolate(double a, double b, double f)
{
    return (b - a) * f + a;
}

inline double dot(const vec3 &a, const vec3 &b)
{
    return a(0) * b(0) + a(1) * b(1) + a(2) * b(2);
}

inline double dot(const vec2 &a, const vec2 &b)
{
    return a(0) * b(0) + a(1) * b(1);
}

inline double length(const vec3 &a)
{
    return std::sqrt(dot(a, a));
}

inline double length(const vec2 &a)
{
    return std::sqrt(dot(a, a));
}

inline vec3 normalize(const vec3 &a)
{
    return a / length(a);
}

inline vec3 min(const vec3 &a, const vec3 &b)
{
    return vec3(std::min(a(0), b(0)),
                std::min(a(1), b(1)),
                std::min(a(2), b(2)));
}

inline vec3 max(const vec3 &a, const vec3 &b)
{
    return vec3(std::max(a(0), b(0)),
                std::max(a(1), b(1)),
                std::max(a(2), b(2)));
}

inline vec3 interpolate(const vec3 &a, const vec3 &b, double f)
{
    return vec3(interpolate(a(0), b(0), f),
                interpolate(a(1), b(1), f),
                interpolate(a(2), b(2), f));
}

inline float dot(const vec3f &a, const vec3f &b)
{
    return a(0) * b(0) + a(1) * b(1) + a(2) * b(2);
}

inline float dot(const vec2f &a, const vec2f &b)
{
    return a(0) * b(0) + a(1) * b(1);
}

inline float length(const vec3f &a)
{
    return std::sqrt(dot(a, a));
}

inline float length(const vec2f &a)
{
    return std::sqrt(dot(a, a));
}

inline vec3f normalize(const vec3f &a)
{
    return a / length(a);
}

inline vec3f min(const vec3f &a, const vec3f &b)
{
    return vec3f(std::min(a(0), b(0)),
        std::min(a(1), b(1)),
        std::min(a(2), b(2)));
}

inline vec3f max(const vec3f &a, const vec3f &b)
{
    return vec3f(std::max(a(0), b(0)),
        std::max(a(1), b(1)),
        std::max(a(2), b(2)));
}

inline vec3f interpolate(const vec3f &a, const vec3f &b, float f)
{
    return vec3f(interpolate(a(0), b(0), f),
        interpolate(a(1), b(1), f),
        interpolate(a(2), b(2), f));
}

inline vec4 vec3to4(const vec3 &v, double w)
{
    vec4 res;
    res[0] = v[0];
    res[1] = v[1];
    res[2] = v[2];
    res[3] = w;
    return res;
}

inline vec3 vec4to3(const vec4 &v, bool division = false)
{
    vec3 res;
    res[0] = v[0];
    res[1] = v[1];
    res[2] = v[2];
    if (division)
        res /= v[3];
    return res;
}

inline vec3 vec2to3(const vec2 &v, double w)
{
    vec3 res;
    res[0] = v[0];
    res[1] = v[1];
    res[2] = w;
    return res;
}

inline vec2 vec3to2(const vec3 &v, bool division = false)
{
    vec2 res;
    res[0] = v[0];
    res[1] = v[1];
    if (division)
        res /= v[2];
    return res;
}

inline vec2 vec4to2(const vec4 &v, bool division = false)
{
    vec2 res;
    res[0] = v[0];
    res[1] = v[1];
    if (division)
        res /= v[3];
    return res;
}

inline vec4f vec3to4(const vec3f &v, float w)
{
    vec4f res;
    res[0] = v[0];
    res[1] = v[1];
    res[2] = v[2];
    res[3] = w;
    return res;
}

inline vec3f vec4to3(const vec4f &v, bool division = false)
{
    vec3f res;
    res[0] = v[0];
    res[1] = v[1];
    res[2] = v[2];
    if (division)
        res /= v[3];
    return res;
}

inline vec3f vec2to3(const vec2f &v, float w)
{
    vec3f res;
    res[0] = v[0];
    res[1] = v[1];
    res[2] = w;
    return res;
}

inline vec2f vec3to2(const vec3f &v, bool division = false)
{
    vec2f res;
    res[0] = v[0];
    res[1] = v[1];
    if (division)
        res /= v[2];
    return res;
}

inline vec2f vec4to2(const vec4f &v, bool division = false)
{
    vec2f res;
    res[0] = v[0];
    res[1] = v[1];
    if (division)
        res /= v[3];
    return res;
}

inline vec2 rawToVec2(const double v[2])
{
    return vec2(v[0], v[1]);
}

inline vec3 rawToVec3(const double v[3])
{
    return vec3(v[0], v[1], v[2]);
}

inline vec4 rawToVec4(const double v[4])
{
    return vec4(v[0], v[1], v[2], v[3]);
}

inline vec2f rawToVec2(const float v[2])
{
    return vec2f(v[0], v[1]);
}

inline vec3f rawToVec3(const float v[3])
{
    return vec3f(v[0], v[1], v[2]);
}

inline vec4f rawToVec4(const float v[4])
{
    return vec4f(v[0], v[1], v[2], v[3]);
}

inline void vecToRaw(const vec2 &a, double v[2])
{
    for (int i = 0; i < 2; i++)
        v[i] = a[i];
}

inline void vecToRaw(const vec3 &a, double v[3])
{
    for (int i = 0; i < 3; i++)
        v[i] = a[i];
}

inline void vecToRaw(const vec4 &a, double v[4])
{
    for (int i = 0; i < 4; i++)
        v[i] = a[i];
}

inline void vecToRaw(const vec2f &a, float v[2])
{
    for (int i = 0; i < 2; i++)
        v[i] = a[i];
}

inline void vecToRaw(const vec3f &a, float v[3])
{
    for (int i = 0; i < 3; i++)
        v[i] = a[i];
}

inline void vecToRaw(const vec4f &a, float v[4])
{
    for (int i = 0; i < 4; i++)
        v[i] = a[i];
}

template<class T, class U>
inline T vecFromUblas(const U &u)
{
    T res;
    for (unsigned i = 0; i < u.size(); i++)
        res(i) = u[i];
    return res;
}

template<class U, class T>
inline U vecToUblas(const T &t)
{
    U res;
    for (unsigned i = 0; i < res.size(); i++)
        res[i] = t(i);
    return res;
}

inline double nan1() { return std::numeric_limits<double>::quiet_NaN(); }
inline vec2 nan2() { return vec2(nan1(), nan1()); }
inline vec3 nan3() { return vec3(nan1(), nan1(), nan1()); }
inline vec4 nan4() { return vec4(nan1(), nan1(), nan1(), nan1()); }

} // namespace vts

#endif // MATH_H_wegfzebvgfhjusd
