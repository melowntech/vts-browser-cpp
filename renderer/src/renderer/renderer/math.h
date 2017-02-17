#ifndef renderer_math_hpp_included_
#define renderer_math_hpp_included_

#include <Eigen/Dense>

#include "foundation.h"

namespace melown
{
    typedef Eigen::Vector2f vec2;
    typedef Eigen::Vector3f vec3;
    typedef Eigen::Vector4f vec4;

    typedef Eigen::Matrix2f mat2;
    typedef Eigen::Matrix3f mat3;
    typedef Eigen::Matrix4f mat4;

    vec3 cross(const vec3 &a, const vec3 &b);
    float dot(const vec3 &a, const vec3 &b);
    float length(const vec3 &a);
    vec3 normalize(const vec3 &a);

    mat4 frustumMatrix(float left, float right, float bottom, float top, float near, float far);
    mat4 perspectiveMatrix(float fovy, float aspect, float near, float far);
    mat4 lookAt(const vec3 &eye, const vec3 &target, const vec3 &up);

    mat4 identityMatrix();
    mat4 rotationMatrix(int axis, float angle);
    mat4 scaleMatrix(float sx, float sy, float sz);
    mat4 scaleMatrix(float s);
    mat4 translationMatrix(float tx, float ty, float tz);
    mat4 translationMatrix(const vec3 &vec);

    const vec3 min(const vec3 &a, const vec3 &b);
    const vec3 max(const vec3 &a, const vec3 &b);

    template<class T, class U>
    const T vecFromUblas(const U &u)
    {
        T res;
        for (int i = 0; i < u.size(); i++)
            res(i) = u[i];
        return res;
    }

    template<class U, class T>
    const U vecToUblas(const T &t)
    {
        U res;
        for (int i = 0; i < res.size(); i++)
            res[i] = t(i);
        return res;
    }

    float degToRad(float angle);
    float radToDeg(float angle);

    const mat3 upperLeftSubMatrix(const mat4 &mat);
}

#endif // renderer_math_hpp_included_
