#include <stdexcept>
#include <QOpenGLDebugLogger>
#include <QImage>
#include <QBuffer>
#include <QImageReader>
#include "gpuContext.hpp"

QOpenGLFunctions_4_4_Core *gpuFunctions()
{
    return QOpenGLContext::currentContext()
            ->versionFunctions<QOpenGLFunctions_4_4_Core>();
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
    qDebug() << message.id() << message.type() << message.message();
}

void GpuShader::bind()
{
    if (!QOpenGLShaderProgram::bind())
        throw std::runtime_error("failed to bind gpu shader");
}

void GpuShader::loadShaders(const std::string &vertexShader,
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

void GpuShader::uniformMat4(vts::uint32 location, const float *value)
{
    QMatrix4x4 m;
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            m(i, j) = value[j * 4 + i];
    setUniformValue(location, m);
}

void GpuShader::uniformMat3(vts::uint32 location, const float *value)
{
    QMatrix3x3 m;
    for (int j = 0; j < 3; j++)
        for (int i = 0; i < 3; i++)
            m(i, j) = value[j * 3 + i];
    setUniformValue(location, m);
}

void GpuShader::uniformVec4(vts::uint32 location, const float *value)
{
    QVector4D m;
    for (int i = 0; i < 4; i++)
        m[i] = value[i];
    setUniformValue(location, m);
}

void GpuShader::uniformVec3(vts::uint32 location, const float *value)
{
    QVector3D m;
    for (int i = 0; i < 3; i++)
        m[i] = value[i];
    setUniformValue(location, m);
}

void GpuShader::uniform(vts::uint32 location, const float value)
{
    setUniformValue(location, value);
}

void GpuShader::uniform(vts::uint32 location, const int value)
{
    setUniformValue(location, value);
}


GpuTextureImpl::GpuTextureImpl(const std::string &name) :
    vts::GpuTexture(name),
    QOpenGLTexture(QOpenGLTexture::Target2D)
{}

void GpuTextureImpl::bind()
{
    QOpenGLTexture::bind();
}

void GpuTextureImpl::loadTexture(const vts::GpuTextureSpec &spec)
{
    QImage::Format format;
    switch (spec.components)
    {
    case 1:
        format = QImage::Format_Grayscale8;
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
    create();
    setData(QImage((unsigned char*)spec.buffer.data(),
                   spec.width, spec.height, format).mirrored());
    setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    setMagnificationFilter(QOpenGLTexture::Linear);
    setWrapMode(QOpenGLTexture::ClampToEdge);
    gpuMemoryCost = spec.buffer.size();
    setMaximumAnisotropy(16);
}

GpuMeshImpl::GpuMeshImpl(const std::string &name) : vts::GpuMesh(name),
    vertexBuffer(QOpenGLBuffer::VertexBuffer),
    indexBuffer(QOpenGLBuffer::IndexBuffer)
{}

void GpuMeshImpl::draw()
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
        for (int i = 0; i < sizeof(vts::GpuMeshSpec::attributes)
             / sizeof(vts::GpuMeshSpec::VertexAttribute); i++)
        {
            vts::GpuMeshSpec::VertexAttribute &a = spec.attributes[i];
            if (a.enable)
            {
                gl->glEnableVertexAttribArray(i);
                gl->glVertexAttribPointer(i, a.components, (GLenum)a.type,
                                          a.normalized ? GL_TRUE : GL_FALSE,
                                          a.stride, (void*)a.offset);
            }
            else
                gl->glDisableVertexAttribArray(i);
        }
    }
    if (spec.indicesCount > 0)
        gl->glDrawElements((GLenum)spec.faceMode, spec.indicesCount,
                           GL_UNSIGNED_SHORT, nullptr);
    else
        gl->glDrawArrays((GLenum)spec.faceMode, 0, spec.verticesCount);
    gl->glBindVertexArray(0);
}

void GpuMeshImpl::loadMesh(const vts::GpuMeshSpec &spec)
{
    this->spec = std::move(spec);
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
    gpuMemoryCost = spec.vertices.size() + spec.indices.size();
    arrayObject.destroy();
}
