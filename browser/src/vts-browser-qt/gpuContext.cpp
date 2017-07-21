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

#include <stdexcept>
#include <QOpenGLDebugLogger>
#include <QImage>
#include <QBuffer>
#include <QImageReader>
#include <sstream>
#include <vts-browser/log.hpp>
#include "gpuContext.hpp"

QOpenGLFunctions_3_3_Core *gpuFunctions()
{
    return QOpenGLContext::currentContext()
            ->versionFunctions<QOpenGLFunctions_3_3_Core>();
}

Gl::Gl(QSurface *surface) : surface(surface)
{
    logger = std::shared_ptr<QOpenGLDebugLogger>(new QOpenGLDebugLogger(this));
}

Gl::~Gl()
{}

void Gl::current(bool bind)
{
    if (bind)
        makeCurrent(surface);
    else
        doneCurrent();
}

void Gl::initialize()
{
    setFormat(surface->format());

    create();
    if (!isValid())
        throw std::runtime_error("unable to create opengl context");

    current(true);
    initializeOpenGLFunctions();

    logger->initialize();
    connect(logger.get(), &QOpenGLDebugLogger::messageLogged,
            this, &Gl::onDebugMessage);
    logger->startLogging();
}

void Gl::onDebugMessage(const QOpenGLDebugMessage &message)
{
    if (message.id() == 131185
            && message.type() == QOpenGLDebugMessage::OtherType)
        return;
    std::stringstream s;
    s << "OpenGL: " << message.id() << ", " << message.type() << ", "
      << message.message().toUtf8().constData();
    vts::log(vts::LogLevel::warn4, s.str());
}

GpuShaderImpl::GpuShaderImpl()
{
    uniformLocations.reserve(20);
}

void GpuShaderImpl::bind()
{
    if (!QOpenGLShaderProgram::bind())
        throw std::runtime_error("failed to bind gpu shader");
}

void GpuShaderImpl::loadShaders(const std::string &vertexShader,
                            const std::string &fragmentShader)
{
    create();
    addShaderFromSourceCode(QOpenGLShader::Vertex,
                            QString::fromUtf8(vertexShader.data(),
                                              vertexShader.size()));
    if (!fragmentShader.empty())
        addShaderFromSourceCode(QOpenGLShader::Fragment,
                                QString::fromUtf8(fragmentShader.data(),
                                                  fragmentShader.size()));
    if (!link())
        throw std::runtime_error("failed to link shader program");
}

void GpuShaderImpl::uniformMat4(vts::uint32 location, const float *value)
{
    QMatrix4x4 m;
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            m(i, j) = value[j * 4 + i];
    setUniformValue(uniformLocations[location], m);
}

void GpuShaderImpl::uniformMat3(vts::uint32 location, const float *value)
{
    QMatrix3x3 m;
    for (int j = 0; j < 3; j++)
        for (int i = 0; i < 3; i++)
            m(i, j) = value[j * 3 + i];
    setUniformValue(uniformLocations[location], m);
}

void GpuShaderImpl::uniformVec4(vts::uint32 location, const float *value)
{
    QVector4D m;
    for (int i = 0; i < 4; i++)
        m[i] = value[i];
    setUniformValue(uniformLocations[location], m);
}

void GpuShaderImpl::uniformVec3(vts::uint32 location, const float *value)
{
    QVector3D m;
    for (int i = 0; i < 3; i++)
        m[i] = value[i];
    setUniformValue(uniformLocations[location], m);
}

void GpuShaderImpl::uniform(vts::uint32 location, float value)
{
    setUniformValue(uniformLocations[location], value);
}

void GpuShaderImpl::uniform(vts::uint32 location, int value)
{
    setUniformValue(uniformLocations[location], value);
}

GpuTextureImpl::GpuTextureImpl() :
    QOpenGLTexture(QOpenGLTexture::Target2D),
    grayscale(false)
{}

void GpuTextureImpl::bind()
{
    QOpenGLTexture::bind();
}

void GpuTextureImpl::loadTexture(vts::ResourceInfo &info,
                                 const vts::GpuTextureSpec &spec)
{
    if (isCreated())
        destroy();
    QImage::Format format;
    grayscale = false;
    switch (spec.components)
    {
    case 1:
        format = QImage::Format_Grayscale8;
        grayscale = true;
        break;
    case 3:
        format = QImage::Format_RGB888;
        break;
    case 4:
        format = QImage::Format_RGBA8888;
        break;
    default:
        throw std::invalid_argument("invalid texture components count");
    }
    setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    setMagnificationFilter(QOpenGLTexture::Linear);
    setWrapMode(QOpenGLTexture::ClampToEdge);
    setMaximumAnisotropy(16);
    create();
    setData(QImage((unsigned char*)spec.buffer.data(),
                   spec.width, spec.height, format));
    info.ramMemoryCost += sizeof(*this);
    info.gpuMemoryCost += spec.buffer.size();
}

GpuMeshImpl::GpuMeshImpl() :
    vertexBuffer(QOpenGLBuffer::VertexBuffer),
    indexBuffer(QOpenGLBuffer::IndexBuffer)
{}

void GpuMeshImpl::draw()
{
    QOpenGLFunctions_3_3_Core *gl = gpuFunctions();
    arrayObject.bind();
    if (spec.indicesCount > 0)
    {
        gl->glDrawElements((GLenum)spec.faceMode, spec.indicesCount,
                           GL_UNSIGNED_SHORT, nullptr);
    }
    else
        gl->glDrawArrays((GLenum)spec.faceMode, 0, spec.verticesCount);
    gl->glBindVertexArray(0);
}

void GpuMeshImpl::loadMesh(vts::ResourceInfo &info,
                           const vts::GpuMeshSpec &specp)
{
    QOpenGLFunctions_3_3_Core *gl = gpuFunctions();

    spec = std::move(specp);

    arrayObject.create();
    arrayObject.bind();

    vertexBuffer.create();
    vertexBuffer.bind();
    vertexBuffer.allocate(spec.vertices.data(), spec.vertices.size());

    if (spec.indicesCount)
    {
        indexBuffer.create();
        indexBuffer.bind();
        indexBuffer.allocate(spec.indices.data(), spec.indices.size());
    }

    for (unsigned i = 0; i < spec.attributes.size(); i++)
    {
        const vts::GpuMeshSpec::VertexAttribute &a = spec.attributes[i];
        if (a.enable)
        {
            gl->glEnableVertexAttribArray(i);
            gl->glVertexAttribPointer(i, a.components, (GLenum)a.type,
                                      a.normalized ? GL_TRUE : GL_FALSE,
                                      a.stride, (void*)(intptr_t)a.offset);
        }
        else
            gl->glDisableVertexAttribArray(i);
    }

    gl->glBindVertexArray(0);

    info.ramMemoryCost += sizeof(*this);
    info.gpuMemoryCost += spec.vertices.size() + spec.indices.size();

    spec.vertices.free();
    spec.indices.free();
}
