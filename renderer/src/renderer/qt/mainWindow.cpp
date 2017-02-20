#include "mainWindow.h"
#include "gpuContext.h"
#include "fetcher.h"

#include <renderer/map.h>

MainWindow::MainWindow() : gl(nullptr)
{
    QSurfaceFormat format;
    format.setVersion(4, 4);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    setFormat(format);

    setSurfaceType(QWindow::OpenGLSurface);
    gl = new Gl(this);

    fetcher = new FetcherImpl();
}

MainWindow::~MainWindow()
{
    delete fetcher;
}

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

    map->dataInitialize(gl, fetcher);
}

void MainWindow::tick()
{
    requestUpdate();
    if (!isExposed())
        return;

    if (!gl->isValid())
        throw "invalid gl context";

    gl->makeCurrent(this);

    QSize size = QWindow::size();
    gl->glViewport(0, 0, size.width(), size.height());
    gl->glClearColor(0.2, 0.2, 0.2, 1);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    map->renderTick(size.width(), size.height());

    map->dataTick();

    gl->swapBuffers(this);
}
