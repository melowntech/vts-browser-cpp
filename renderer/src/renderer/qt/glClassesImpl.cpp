#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QImage>
#include <QBuffer>
#include <QImageReader>

#include "glContextImpl.h"

#include <renderer/glClasses.h>

class GlShaderImpl : public QOpenGLShaderProgram, public melown::GlShader
{
public:
    GlShaderImpl()
    {
        create();
    }

    melown::uint32 memoryCost() const override
    {
        return 0;
    }

    bool ready() const override
    {
        return true;
    }

    void bind() override
    {
        QOpenGLShaderProgram::bind();
    }

    void loadToGpu(const std::string &vertexShader, const std::string &fragmentShader) override
    {
        addShaderFromSourceCode(QOpenGLShader::Vertex, QString((QChar*)vertexShader.data(), vertexShader.size()));
        if (!fragmentShader.empty())
            addShaderFromSourceCode(QOpenGLShader::Fragment, QString((QChar*)fragmentShader.data(), fragmentShader.size()));
        link();
        removeAllShaders();
    }
};


class GlTextureImpl : public QOpenGLTexture, public melown::GlTexture
{
public:
    GlTextureImpl() : QOpenGLTexture(QOpenGLTexture::Target2D), loaded(false), cost(0)
    {
        create();
    }

    melown::uint32 memoryCost() const override
    {
        return 0;
    }

    bool ready() const override
    {
        return loaded;
    }

    void bind() override
    {
        QOpenGLTexture::bind();
    }

    void loadToGpu(void *buffer, melown::uint32 size) override
    {
        QByteArray arr((char*)buffer, size);
        QBuffer buff(&arr);
        QImageReader reader;
        reader.setDevice(&buff);
        reader.autoDetectImageFormat();
        QImage img(reader.read());
        setData(img);
        loaded = true;
        cost = img.byteCount();
    }

    bool loaded;
    melown::uint32 cost;
};


class GlMeshImpl : public QObject, public melown::GlMesh
{
public:
    GlMeshImpl()
    {

    }

    melown::uint32 memoryCost() const override
    {
        return 0;
    }

    bool ready() const override
    {
        return false;
    }

    void draw() override
    {

    }
};

std::shared_ptr<melown::GlShader> Gl::createShader()
{
    return std::shared_ptr<melown::GlShader>(new GlShaderImpl());
}

std::shared_ptr<melown::GlTexture> Gl::createTexture()
{
    return std::shared_ptr<melown::GlTexture>(new GlTextureImpl());
}

std::shared_ptr<melown::GlMesh> Gl::createMesh()
{
    return std::shared_ptr<melown::GlMesh>(new GlMeshImpl());
}
