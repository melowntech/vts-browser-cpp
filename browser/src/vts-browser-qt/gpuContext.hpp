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

#ifndef GLCONTEXTIMPL_H_jkehrbhejkfn
#define GLCONTEXTIMPL_H_jkehrbhejkfn

#include <memory>
#include <string>
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QSurface>
#include <vts-browser/resources.hpp>

class Gl : public QOpenGLContext, public QOpenGLFunctions_3_3_Core
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

class GpuShaderImpl : public QOpenGLShaderProgram
{
public:
    GpuShaderImpl();

    void bind();
    void loadShaders(const std::string &vertexShader,
                     const std::string &fragmentShader);
    void uniformMat4(vts::uint32 location, const float *value);
    void uniformMat3(vts::uint32 location, const float *value);
    void uniformVec4(vts::uint32 location, const float *value);
    void uniformVec3(vts::uint32 location, const float *value);
    void uniform(vts::uint32 location, float value);
    void uniform(vts::uint32 location, int value);

    std::vector<vts::uint32> uniformLocations;
};

class GpuTextureImpl : public QOpenGLTexture
{
public:
    GpuTextureImpl();

    void bind();
    void loadTexture(vts::ResourceInfo &info, const vts::GpuTextureSpec &spec);

    bool grayscale;
};

class GpuMeshImpl
{
public:
    GpuMeshImpl();

    void draw();
    void loadMesh(vts::ResourceInfo &info, const vts::GpuMeshSpec &spec);

    QOpenGLVertexArrayObject arrayObject;
    QOpenGLBuffer vertexBuffer;
    QOpenGLBuffer indexBuffer;
    vts::GpuMeshSpec spec;
};

#endif
