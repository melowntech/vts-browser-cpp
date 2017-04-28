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
#include <vts/resources.hpp>

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
    void uniformMat4(vts::uint32 location, const float *value);
    void uniformMat3(vts::uint32 location, const float *value);
    void uniformVec4(vts::uint32 location, const float *value);
    void uniformVec3(vts::uint32 location, const float *value);
    void uniform(vts::uint32 location, const float value);
    void uniform(vts::uint32 location, const int value);
};

class GpuTextureImpl : public QOpenGLTexture, public vts::GpuTexture
{
public:
    GpuTextureImpl(const std::string &name);
    void bind();
    void loadTexture(const vts::GpuTextureSpec &spec);
    bool grayscale;
};

class GpuMeshImpl : public vts::GpuMesh
{
public:
    GpuMeshImpl(const std::string &name);
    void draw();
    void loadMesh(const vts::GpuMeshSpec &spec);

    QOpenGLVertexArrayObject arrayObject;
    QOpenGLBuffer vertexBuffer;
    QOpenGLBuffer indexBuffer;
    vts::GpuMeshSpec spec;
};

#endif
