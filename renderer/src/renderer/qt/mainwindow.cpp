#include "mainwindow.h"
#include "glContextImpl.h"

#include <renderer/map.h>

MainWindow::MainWindow() : gl(nullptr), glInitialized(false)
{
    setSurfaceType(QWindow::OpenGLSurface);
    gl = new Gl(this);
}

MainWindow::~MainWindow()
{}

bool MainWindow::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::UpdateRequest:
        tick();
        return true;
    default:
        return QWindow::event(event);
    }
}

void MainWindow::tick()
{
    requestUpdate();
    if (!isExposed())
        return;

    if (glInitialized)
        gl->makeCurrent(this);
    else
    {
        gl->initialize();
        glInitialized = true;
    }

    { // temporarily simulate render by insanely flickering screen
        gl->glClearColor(qrand() / (float)RAND_MAX, 0, 0, 0);
        gl->glClear(GL_COLOR_BUFFER_BIT);
    }
    map->render();

    gl->swapBuffers(this);
}
