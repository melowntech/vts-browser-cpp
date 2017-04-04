#ifndef renderer_math_hpp_included_
#define renderer_math_hpp_included_

#include <Eigen/Dense>

#include <melown/foundation.hpp>

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

const vec3 cross(const vec3 &a, const vec3 &b);
double dot(const vec3 &a, const vec3 &b);
double dot(const vec2 &a, const vec2 &b);
double length(const vec3 &a);
double length(const vec2 &a);
const vec3 normalize(const vec3 &a);

const mat4 frustumMatrix(double left, double right, double bottom, double top,
                         double near, double far);
const mat4 perspectiveMatrix(double fovyDegs, double aspect,
                             double near, double far);
const mat4 lookAt(const vec3 &eye, const vec3 &target, const vec3 &up);

const mat4 identityMatrix();
const mat4 rotationMatrix(int axis, double radians);
const mat4 scaleMatrix(double sx, double sy, double sz);
const mat4 scaleMatrix(double s);
const mat4 translationMatrix(double tx, double ty, double tz);
const mat4 translationMatrix(const vec3 &vec);

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

const vec4 vec3to4(vec3 v, double w);
const vec3 vec4to3(vec4 v, bool division = false);
const vec3 vec2to3(vec2 v, double w);
const vec2 vec3to2(vec3 v, bool division = false);

double modulo(double a, double m);
double interpolate(double a, double b, double f);
double clamp(double a, double min, double max);

const vec3 lowerUpperCombine(uint32 i);
const vec4 column(const mat4 &m, uint32 index);

} // namespace melown

#endif // renderer_math_hpp_included_
