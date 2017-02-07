#include <QOpenGLFunctions_4_4_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QImage>
#include <QBuffer>
#include <QImageReader>

#include "glData.h"

using namespace melown;

class GlShaderImpl : public QOpenGLShaderProgram, public GlShader
{
public:
    GlShaderImpl(GlData *gl) : gl(gl)
    {
        create();
    }

    virtual uint32 memoryCost() const
    {
        return 0;
    }

    virtual bool ready() const
    {
        return true;
    }

    virtual void bind()
    {
        QOpenGLShaderProgram::bind();
    }

    virtual void loadToGpu(const std::string &vertexShader, const std::string &fragmentShader)
    {
        addShaderFromSourceCode(QOpenGLShader::Vertex, QString((QChar*)vertexShader.data(), vertexShader.size()));
        if (!fragmentShader.empty())
            addShaderFromSourceCode(QOpenGLShader::Fragment, QString((QChar*)fragmentShader.data(), fragmentShader.size()));
        link();
        removeAllShaders();
    }

    GlData *gl;
};


class GlTextureImpl : public QOpenGLTexture, public GlTexture
{
public:
    GlTextureImpl(GlData *gl) : QOpenGLTexture(QOpenGLTexture::Target2D), gl(gl), loaded(false), cost(0)
    {
        create();
    }

    virtual uint32 memoryCost() const
    {
        return 0;
    }

    virtual bool ready() const
    {
        return loaded;
    }

    virtual void bind()
    {
        QOpenGLTexture::bind();
    }

    virtual void loadToGpu(void *buffer, uint32 size)
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

    GlData *gl;
    bool loaded;
    uint32 cost;
};


class GlMeshImpl : public QObject, public GlMesh
{
public:
    GlMeshImpl(GlData *gl) : gl(gl)
    {

    }

    virtual uint32 memoryCost() const
    {
        return 0;
    }

    virtual bool ready() const
    {
        return false;
    }

    virtual void draw()
    {

    }

    GlData *gl;
};

std::shared_ptr<GlShader> GlData::createShader()
{
    return std::shared_ptr<GlShader>(new GlShaderImpl(this));
}

std::shared_ptr<GlTexture> GlData::createTexture()
{
    return std::shared_ptr<GlTexture>(new GlTextureImpl(this));
}

std::shared_ptr<GlMesh> GlData::createMesh()
{
    return std::shared_ptr<GlMesh>(new GlMeshImpl(this));
}
