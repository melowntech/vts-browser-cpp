#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
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
        if (!link())
            throw "failed to link shader program";
        //removeAllShaders();
        ready = true;
    }


    void uniform(melown::uint32 location, const melown::mat3 &v) override
    {
        QMatrix3x3 m;
        for (int j = 0; j < 3; j++)
        for (int i = 0; i < 3; i++)
            m(i, j) = v(i, j);
        setUniformValue(location, m);
    }

    void uniform(melown::uint32 location, const melown::mat4 &v) override
    {
        QMatrix4x4 m;
        for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            m(i, j) = v(i, j);
        setUniformValue(location, m);
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


class GpuSubMeshImpl : public QObject, public melown::GpuMeshRenderable
{
public:
    GpuSubMeshImpl() : vertexBuffer(QOpenGLBuffer::VertexBuffer), indexBuffer(QOpenGLBuffer::IndexBuffer)
    {}

    void draw() override
    {
        QOpenGLFunctions_4_4_Core *gl = gpuFunctions();
        if (arrayObject.isCreated())
            arrayObject.bind();
        else
        {
            arrayObject.create();
            arrayObject.bind();
            vertexBuffer.bind();
            if (indexBuffer.isCreated())
                indexBuffer.bind();
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
        }
        if (spec.indexCount > 0)
            gl->glDrawElements((GLenum)spec.faceMode, spec.indexCount, GL_UNSIGNED_SHORT, nullptr);
        else
            gl->glDrawArrays((GLenum)spec.faceMode, 0, spec.vertexCount);
        gl->glBindVertexArray(0);
    }

    void loadMeshRenderable(const melown::GpuMeshSpec &spec) override
    {
        this->spec = spec;
        arrayObject.create();
        arrayObject.bind();
        vertexBuffer.create();
        vertexBuffer.bind();
        vertexBuffer.allocate(spec.vertexBuffer, spec.vertexSize * spec.vertexCount);
        if (spec.indexCount)
        {
            indexBuffer.create();
            indexBuffer.bind();
            indexBuffer.allocate(spec.indexBuffer, spec.indexCount * sizeof(melown::uint16));
        }
        gpuMemoryCost = spec.vertexSize * spec.vertexCount + spec.indexCount * sizeof(melown::uint16);
        arrayObject.destroy();
        ready = true;
        this->spec.vertexBuffer = nullptr;
        this->spec.indexBuffer = nullptr;
    }

    QOpenGLVertexArrayObject arrayObject;
    QOpenGLBuffer vertexBuffer;
    QOpenGLBuffer indexBuffer;
    melown::GpuMeshSpec spec;
};

std::shared_ptr<melown::Resource> Gl::createShader()
{
    return std::shared_ptr<melown::GpuShader>(new GpuShaderImpl());
}

std::shared_ptr<melown::Resource> Gl::createTexture()
{
    return std::shared_ptr<melown::GpuTexture>(new GpuTextureImpl());
}

std::shared_ptr<melown::Resource> Gl::createMeshRenderable()
{
    return std::shared_ptr<melown::GpuMeshRenderable>(new GpuSubMeshImpl());
}
