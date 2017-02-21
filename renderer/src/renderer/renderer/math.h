#ifndef renderer_math_hpp_included_
#define renderer_math_hpp_included_

#include <Eigen/Dense>

#include <renderer/foundation.h>

namespace melown
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

    vec3 cross(const vec3 &a, const vec3 &b);
    double dot(const vec3 &a, const vec3 &b);
    double length(const vec3 &a);
    vec3 normalize(const vec3 &a);

    mat4 frustumMatrix(double left, double right, double bottom, double top, double near, double far);
    mat4 perspectiveMatrix(double fovy, double aspect, double near, double far);
    mat4 lookAt(const vec3 &eye, const vec3 &target, const vec3 &up);

    mat4 identityMatrix();
    mat4 rotationMatrix(int axis, double angle);
    mat4 scaleMatrix(double sx, double sy, double sz);
    mat4 scaleMatrix(double s);
    mat4 translationMatrix(double tx, double ty, double tz);
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

    double degToRad(double angle);
    double radToDeg(double angle);

    const mat3 upperLeftSubMatrix(const mat4 &mat);
    const vec3 vec4to3(const vec4 &value);
}

#endif // renderer_math_hpp_included_
