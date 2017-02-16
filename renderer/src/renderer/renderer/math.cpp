#include "math.h"

namespace melown
{
    vec3 cross(const vec3 &a, const vec3 &b)
    {
        return vec3(
                    a(1) * b(2) - a(2) * b(1),
                    a(2) * b(0) - a(0) * b(2),
                    a(0) * b(1) - a(1) * b(0)
                    );
    }

    float dot(const vec3 &a, const vec3 &b)
    {
        return a(0) * b(0) + a(1) * b(1) + a(2) * b(2);
    }

    float length(const vec3 &a)
    {
        return sqrtf(dot(a, a));
    }

    vec3 normalize(const vec3 &a)
    {
        return a / length(a);
    }

    mat4 frustumMatrix(float left, float right, float bottom, float top, float near, float far)
    {
        float w(right - left);
        float h(top - bottom);
        float d(far - near);

        return (mat4() <<
                2*near/w, 0, (right+left)/w, 0,
                0, 2*near/h, (top+bottom)/h, 0,
                0, 0, -(far+near)/d, -2*far*near/d,
                0, 0, -1, 0).finished();
    }

    mat4 perspectiveMatrix(float fovy, float aspect, float near, float far)
    {
        float ymax = near * tanf(fovy * M_PI / 360.0);
        float xmax = ymax * aspect;
        return frustumMatrix(-xmax, xmax, -ymax, ymax, near, far);
    }


    inline float &at(mat4 &a, uint32 i)
    {
        return a(i % 4, i / 4);
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

    mat4 identityMatrix()
    {
        return scaleMatrix(1);
    }

    mat4 rotationMatrix(int axis, float angle)
    {
        float ca(cos(angle)), sa(sin(angle));

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
        default:
            return (mat4() <<
                    ca,-sa, 0, 0,
                    sa, ca, 0, 0,
                    0,  0,  1, 0,
                    0,  0,  0, 1).finished();
        }
    }

    mat4 scaleMatrix(float sx, float sy, float sz)
    {
        return (mat4() <<
                sx,  0,  0, 0,
                0, sy,  0, 0,
                0,  0, sz, 0,
                0,  0,  0, 1).finished();
    }

    mat4 scaleMatrix(float s)
    {
        return scaleMatrix(s, s, s);
    }

    mat4 translationMatrix(float tx, float ty, float tz)
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

    const vec3 min(const vec3 &a, const vec3 &b)
    {
        return vec3(std::min(a(0), b(0)), std::min(a(1), b(1)), std::min(a(2), b(2)));
    }

    const vec3 max(const vec3 &a, const vec3 &b)
    {
        return vec3(std::max(a(0), b(0)), std::max(a(1), b(1)), std::max(a(2), b(2)));
    }
}
