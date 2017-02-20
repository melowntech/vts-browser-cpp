#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QImage>
#include <QBuffer>
#include <QImageReader>

#include "gpuContext.h"

#include <renderer/gpuResources.h>

QOpenGLFunctions_4_4_Core *gpuFunctions()
{
    return QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_4_Core>();
}

class GpuShaderImpl : public QOpenGLShaderProgram, public melown::GpuShader
{
public:
    GpuShaderImpl(const std::string &name) : melown::GpuShader(name)
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
        state = melown::Resource::State::ready;
    }

    void uniformMat4(melown::uint32 location, const float *value) override
    {
        QMatrix4x4 m;
        for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            m(i, j) = value[j * 4 + i];
        setUniformValue(location, m);
    }
};


class GpuTextureImpl : public QOpenGLTexture, public melown::GpuTexture
{
public:
    GpuTextureImpl(const std::string &name) : melown::GpuTexture(name), QOpenGLTexture(QOpenGLTexture::Target2D)
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
        state = melown::Resource::State::ready;
    }
};


class GpuSubMeshImpl : public QObject, public melown::GpuMeshRenderable
{
public:
    GpuSubMeshImpl(const std::string &name) : melown::GpuMeshRenderable(name), vertexBuffer(QOpenGLBuffer::VertexBuffer), indexBuffer(QOpenGLBuffer::IndexBuffer)
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
        if (spec.indicesCount > 0)
            gl->glDrawElements((GLenum)spec.faceMode, spec.indicesCount, GL_UNSIGNED_SHORT, nullptr);
        else
            gl->glDrawArrays((GLenum)spec.faceMode, 0, spec.verticesCount);
        gl->glBindVertexArray(0);
    }

    void loadMeshRenderable(const melown::GpuMeshSpec &spec) override
    {
        this->spec = spec;
        arrayObject.create();
        arrayObject.bind();
        vertexBuffer.create();
        vertexBuffer.bind();
        vertexBuffer.allocate(spec.vertexBufferData, spec.vertexBufferSize);
        if (spec.indicesCount)
        {
            indexBuffer.create();
            indexBuffer.bind();
            indexBuffer.allocate(spec.indexBufferData, spec.indicesCount * sizeof(melown::uint16));
        }
        gpuMemoryCost = spec.vertexBufferSize + spec.indicesCount * sizeof(melown::uint16);
        arrayObject.destroy();
        this->spec.vertexBufferData = nullptr;
        this->spec.indexBufferData = nullptr;
        state = melown::Resource::State::ready;
    }

    QOpenGLVertexArrayObject arrayObject;
    QOpenGLBuffer vertexBuffer;
    QOpenGLBuffer indexBuffer;
    melown::GpuMeshSpec spec;
};

std::shared_ptr<melown::Resource> Gl::createShader(const std::string &name)
{
    return std::shared_ptr<melown::GpuShader>(new GpuShaderImpl(name));
}

std::shared_ptr<melown::Resource> Gl::createTexture(const std::string &name)
{
    return std::shared_ptr<melown::GpuTexture>(new GpuTextureImpl(name));
}

std::shared_ptr<melown::Resource> Gl::createMeshRenderable(const std::string &name)
{
    return std::shared_ptr<melown::GpuMeshRenderable>(new GpuSubMeshImpl(name));
}
