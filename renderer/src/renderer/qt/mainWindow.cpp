#include "mainWindow.h"
#include "gpuContext.h"

#include "../renderer/map.h"

MainWindow::MainWindow() : gl(nullptr)
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

void MainWindow::initialize()
{
    gl->initialize();
    map->renderInitialize(gl);
    GLuint a;
    gl->glGenVertexArrays(1, &a);
    gl->glBindVertexArray(a);
}

void MainWindow::tick()
{
    requestUpdate();
    if (!isExposed())
        return;

    gl->makeCurrent(this);

    { // temporarily simulate render by insanely flickering screen
        gl->glClearColor(qrand() / (float)RAND_MAX, 0, 0, 0);
        gl->glClear(GL_COLOR_BUFFER_BIT);
    }
    map->renderTick();

    gl->swapBuffers(this);
}
