#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QImage>
#include <QBuffer>
#include <QImageReader>

#include "gpuContext.h"

#include "../renderer/gpuResources.h"

QOpenGLFunctions_4_4_Core *gpuFunctions()
{
    return QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_4_Core>();
}

class GpuShaderImpl : public QOpenGLShaderProgram, public melown::GpuShader
{
public:
    GpuShaderImpl()
    {}

    void bind() override
    {
        if (!QOpenGLShaderProgram::bind())
            throw "failed to bind gpu shader";
    }

    void loadShaders(const std::string &vertexShader, const std::string &fragmentShader) override
    {
        create();
        addShaderFromSourceCode(QOpenGLShader::Vertex, QString::fromUtf8(vertexShader.data(), vertexShader.size()));
        if (!fragmentShader.empty())
            addShaderFromSourceCode(QOpenGLShader::Fragment, QString::fromUtf8(fragmentShader.data(), fragmentShader.size()));
        link();
        removeAllShaders();
        ready = true;
    }
};


class GpuTextureImpl : public QOpenGLTexture, public melown::GpuTexture
{
public:
    GpuTextureImpl() : QOpenGLTexture(QOpenGLTexture::Target2D)
    {}

    void bind() override
    {
        QOpenGLTexture::bind();
    }

    void loadTexture(void *buffer, melown::uint32 size) override
    {
        create();
        QByteArray arr((char*)buffer, size);
        QBuffer buff(&arr);
        QImageReader reader;
        reader.setDevice(&buff);
        reader.autoDetectImageFormat();
        QImage img(reader.read());
        setData(img);
        gpuMemoryCost = img.byteCount();
        ready = true;
    }
};


class GpuSubMeshImpl : public QObject, public melown::GpuSubMesh
{
public:
    GpuSubMeshImpl() : vertexBuffer(QOpenGLBuffer::VertexBuffer), indexBuffer(QOpenGLBuffer::IndexBuffer)
    {}

    void draw() override
    {
        vertexBuffer.bind();
        indexBuffer.bind();
        QOpenGLFunctions_4_4_Core *gl = gpuFunctions();
        for (int i = 0; i < sizeof(melown::GpuMeshSpec::attributes) / sizeof(melown::GpuMeshSpec::VertexAttribute); i++)
        {
            melown::GpuMeshSpec::VertexAttribute &a = spec.attributes[i];
            if (a.enable)
            {
                gl->glEnableVertexAttribArray(i);
                gl->glVertexAttribPointer(i, a.components, (GLenum)a.type, a.normalized ? GL_TRUE : GL_FALSE, a.stride, (void*)a.offset);
            }
            else
                gl->glDisableVertexAttribArray(i);
        }
        if (spec.indexCount)
            gl->glDrawElements((GLenum)spec.faceMode, spec.indexCount, GL_UNSIGNED_SHORT, nullptr);
        else
            gl->glDrawArrays((GLenum)spec.faceMode, 0, spec.indexCount);
    }

    void loadSubMesh(const melown::GpuMeshSpec &spec) override
    {
        this->spec = spec;
        vertexBuffer.create();
        vertexBuffer.bind();
        vertexBuffer.allocate(spec.vertexBuffer, spec.vertexSize * spec.vertexCount);
        indexBuffer.create();
        indexBuffer.bind();
        indexBuffer.allocate(spec.indexBuffer, spec.indexCount * sizeof(melown::uint16));
        gpuMemoryCost = spec.vertexSize * spec.vertexCount + spec.indexCount * sizeof(melown::uint16);
        ready = true;
    }

    QOpenGLBuffer vertexBuffer;
    QOpenGLBuffer indexBuffer;
    melown::GpuMeshSpec spec;
};

class GpuMeshAggregateImpl : public QObject, public melown::GpuMeshAggregate
{
public:
    GpuMeshAggregateImpl()
    {}

    void loadMeshAggregate() override
    {
        ready = true;
        gpuMemoryCost = 0;
        for (auto it : submeshes)
        {
            ready = ready && it->ready;
            gpuMemoryCost += it->gpuMemoryCost;
        }
    }
};

std::shared_ptr<melown::Resource> Gl::createShader()
{
    return std::shared_ptr<melown::GpuShader>(new GpuShaderImpl());
}

std::shared_ptr<melown::Resource> Gl::createTexture()
{
    return std::shared_ptr<melown::GpuTexture>(new GpuTextureImpl());
}

std::shared_ptr<melown::Resource> Gl::createSubMesh()
{
    return std::shared_ptr<melown::GpuSubMesh>(new GpuSubMeshImpl());
}

std::shared_ptr<melown::Resource> Gl::createMeshAggregate()
{
    return std::shared_ptr<melown::GpuMeshAggregate>(new GpuMeshAggregateImpl());
}
