#ifndef RENDERER_H
#define RENDERER_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_4_Core>
#include <QOpenGLDebugLogger>

class Renderer : public QOpenGLWidget, protected QOpenGLFunctions_4_4_Core
{
public:
    Renderer(QWidget *parent);
    ~Renderer();

protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

    void debugLogMessage(const QOpenGLDebugMessage &msg);
    void onTimer();

    QOpenGLDebugLogger *logger;
};

#endif // RENDERER_H
