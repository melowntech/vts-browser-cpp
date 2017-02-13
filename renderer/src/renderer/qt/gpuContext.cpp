#include "gpuContext.h"

#include <QOpenGLDebugLogger>
#include <QSurface>

Gl::Gl(QSurface *surface) : surface(surface), logger(nullptr)
{
    logger = new QOpenGLDebugLogger(this);
}

Gl::~Gl()
{}

void Gl::initialize()
{
    qDebug() << "initializing opengl context";

    QSurfaceFormat format;
    format.setVersion(4, 4);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);

    create();
    if (!isValid())
        throw "unable to create opengl context";

    makeCurrent(surface);
    initializeOpenGLFunctions();

    logger->initialize();
    connect(logger, &QOpenGLDebugLogger::messageLogged, this, &Gl::onDebugMessage);
    logger->startLogging();
}

void Gl::onDebugMessage(const QOpenGLDebugMessage &message)
{
    qDebug() << message.message();
}
