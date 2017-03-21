#include "math.h"

namespace melown
{

const vec3 cross(const vec3 &a, const vec3 &b)
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

const vec3 normalize(const vec3 &a)
{
    return a / length(a);
}

const mat4 frustumMatrix(double left, double right,
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

const mat4 perspectiveMatrix(double fovyDegs, double aspect, double near, double far)
{
    double ymax = near * tanf(fovyDegs * M_PI / 360.0);
    double xmax = ymax * aspect;
    return frustumMatrix(-xmax, xmax, -ymax, ymax, near, far);
}

namespace {
inline double &at(mat3 &a, uint32 i)
{
    return a(i % 3, i / 3);
}

inline double &at(mat4 &a, uint32 i)
{
    return a(i % 4, i / 4);
}
}

const mat4 lookAt(const vec3 &eye, const vec3 &target, const vec3 &up)
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

const mat4 identityMatrix()
{
    return scaleMatrix(1);
}

const mat4 rotationMatrix(int axis, double radians)
{
    double ca(cos(radians)), sa(sin(radians));

    switch (axis) {
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
        throw "invalid rotation matrix axis";
    }
}

const mat4 scaleMatrix(double sx, double sy, double sz)
{
    return (mat4() <<
            sx,  0,  0, 0,
            0, sy,  0, 0,
            0,  0, sz, 0,
            0,  0,  0, 1).finished();
}

const mat4 scaleMatrix(double s)
{
    return scaleMatrix(s, s, s);
}

const mat4 translationMatrix(double tx, double ty, double tz)
{
    return (mat4() <<
            1, 0, 0, tx,
            0, 1, 0, ty,
            0, 0, 1, tz,
            0, 0, 0, 1).finished();
}

const mat4 translationMatrix(const vec3 &vec)
{
    return translationMatrix(vec(0), vec(1), vec(2));
}

const vec3 min(const vec3 &a, const vec3 &b)
{
    return vec3(std::min(a(0), b(0)),
                std::min(a(1), b(1)),
                std::min(a(2), b(2)));
}

const vec3 max(const vec3 &a, const vec3 &b)
{
    return vec3(std::max(a(0), b(0)),
                std::max(a(1), b(1)),
                std::max(a(2), b(2)));
}


double degToRad(double angle)
{
    return angle * M_PI / 180;
}

double radToDeg(double angle)
{
    return angle * 180 / M_PI;
}

const mat3 upperLeftSubMatrix(const mat4 &m)
{
    mat4 mat(m);
    mat3 res;
    at(res, 0) = at(mat, 0);
    at(res, 1) = at(mat, 1);
    at(res, 2) = at(mat, 2);
    at(res, 3) = at(mat, 4);
    at(res, 4) = at(mat, 5);
    at(res, 5) = at(mat, 6);
    at(res, 6) = at(mat, 8);
    at(res, 7) = at(mat, 9);
    at(res, 8) = at(mat, 10);
    return res;
}

double modulo(double a, double m)
{
    int b = (int)(a / m);
    return a - m * b;
}

const vec4 vec3to4(vec3 v, double w)
{
    vec4 res;
    res(0) = v(0);
    res(1) = v(1);
    res(2) = v(2);
    res(3) = w;
    return res;
}

const vec3 vec4to3(vec4 v, bool division)
{
    vec3 res;
    if (division)
        v = v / v(3);
    res(0) = v(0);
    res(1) = v(1);
    res(2) = v(2);
    return res;
}

const vec3 vec2to3(vec2 v, double w)
{
    vec3 res;
    res(0) = v(0);
    res(1) = v(1);
    res(2) = w;
    return res;
}

const vec2 vec3to2(vec3 v, bool division)
{
    vec2 res;
    if (division)
        v = v / v(2);
    res(0) = v(0);
    res(1) = v(1);
    return res;
}

const vec3 lowerUpperCombine(uint32 i)
{
    vec3 res;
    res(0) = (i >> 0) % 2;
    res(1) = (i >> 1) % 2;
    res(2) = (i >> 2) % 2;
    return res;
}

const vec4 column(const mat4 &m, uint32 index)
{
    return vec4(m(index, 0), m(index, 1), m(index, 2), m(index, 3));
}

} // namespace melown

