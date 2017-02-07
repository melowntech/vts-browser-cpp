#include "glContext.h"

GlContext::GlContext(QWidget *parent) : QOpenGLWidget(parent), logger(nullptr), map(nullptr)
{
    logger = new QOpenGLDebugLogger(this);
}

GlContext::~GlContext()
{
    delete logger;
}

void GlContext::setMap(melown::Map *map)
{
    this->map = map;
}

void GlContext::debugLogMessage(const QOpenGLDebugMessage &msg)
{
    qDebug() << msg.message();
}

void GlContext::initializeGL()
{
    initializeOpenGLFunctions();
    logger->initialize();
    connect(logger, &QOpenGLDebugLogger::messageLogged, this, &GlContext::debugLogMessage);
    logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
}

void GlContext::resizeGL(int w, int h)
{}

void GlContext::paintGL()
{}
