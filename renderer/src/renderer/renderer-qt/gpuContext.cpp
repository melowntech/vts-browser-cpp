#include <QOpenGLDebugLogger>
#include <QSurface>

#include "gpuContext.h"

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
        throw "unable to create opengl context";

    current(true);
    initializeOpenGLFunctions();

    logger->initialize();
    connect(logger.get(), &QOpenGLDebugLogger::messageLogged,
            this, &Gl::onDebugMessage);
    logger->startLogging();
}

void Gl::onDebugMessage(const QOpenGLDebugMessage &message)
{
    //qDebug() << message.id() << " " << message.type();
    if (message.id() == 131185
            && message.type() == QOpenGLDebugMessage::OtherType)
        return;
    qDebug() << message.message();
}


void Gl::wiremode(bool wiremode)
{
    if (wiremode)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
