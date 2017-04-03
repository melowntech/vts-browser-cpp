#ifndef GLCONTEXTIMPL_H_jkehrbhejkfn
#define GLCONTEXTIMPL_H_jkehrbhejkfn

#include <memory>
#include <string>
#include <QOpenGLContext>
#include <QOpenGLFunctions_4_4_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QSurface>
#include <melown/resources.hpp>

class Gl : public QOpenGLContext, public QOpenGLFunctions_4_4_Core
{
public:
    Gl(class QSurface *surface);
    ~Gl();

    void initialize();
    void current(bool bind = true);

    void onDebugMessage(const class QOpenGLDebugMessage &message);

    std::shared_ptr<class QOpenGLDebugLogger> logger;

    QSurface *surface;
};

class GpuShader : public QOpenGLShaderProgram
{
public:
    void bind();
    void loadShaders(const std::string &vertexShader,
                     const std::string &fragmentShader);
    void uniformMat4(melown::uint32 location, const float *value);
    void uniformMat3(melown::uint32 location, const float *value);
    void uniformVec4(melown::uint32 location, const float *value);
    void uniformVec3(melown::uint32 location, const float *value);
    void uniform(melown::uint32 location, const float value);
    void uniform(melown::uint32 location, const int value);
};

class GpuTextureImpl : public QOpenGLTexture, public melown::GpuTexture
{
public:
    GpuTextureImpl(const std::string &name);
    void bind();
    void loadTexture(const melown::GpuTextureSpec &spec);
};

class GpuMeshImpl : public melown::GpuMesh
{
public:
    GpuMeshImpl(const std::string &name);
    void draw();
    void loadMesh(const melown::GpuMeshSpec &spec);

    QOpenGLVertexArrayObject arrayObject;
    QOpenGLBuffer vertexBuffer;
    QOpenGLBuffer indexBuffer;
    melown::GpuMeshSpec spec;
};

#endif
