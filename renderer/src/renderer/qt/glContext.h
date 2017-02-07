#ifndef GLCONTEXT_H
#define GLCONTEXT_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_4_Core>
#include <QOpenGLDebugLogger>

#include <renderer/glContext.h>
#include <renderer/glClasses.h>
#include <renderer/map.h>

class GlContext : public QOpenGLWidget, protected QOpenGLFunctions_4_4_Core, public melown::GlContext
{
public:
    GlContext(QWidget *parent);
    ~GlContext();

    void setMap(melown::Map *map);

protected:
    melown::Map *map;
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

private:
    void debugLogMessage(const QOpenGLDebugMessage &msg);
    QOpenGLDebugLogger *logger;
};

#endif // GLCONTEXT_H
