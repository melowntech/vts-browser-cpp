#include <QGuiApplication>
#include <QMouseEvent>
#include <QDebug>

#include "mainWindow.h"
#include "gpuContext.h"
#include "fetcher.h"

#include <renderer/map.h>

MainWindow::MainWindow() : gl(nullptr), isMouseDetached(false)
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

void MainWindow::mouseMove(QMouseEvent *event)
{
    if (isMouseDetached)
    {
        QPoint diff = event->globalPos() - mouseLastPosition;
        if (event->buttons() & Qt::LeftButton)
        { // pan
            double value[3] = {-diff.x(), diff.y(), 0};
            map->pan(value);
        }
        if (event->buttons() & Qt::MidButton)
        { // rotate
            double value[3] = {diff.x() * 0.1, diff.y() * -0.1, 0};
            map->rotate(value);
        }
        QCursor::setPos(mouseLastPosition);
    }
}

void MainWindow::mousePress(QMouseEvent *event)
{
    if (!isMouseDetached)
    {
        isMouseDetached = true;
        mouseOriginalPosition = event->globalPos();
        mouseLastPosition = geometry().center();
        QGuiApplication::setOverrideCursor(Qt::BlankCursor);
        QCursor::setPos(mouseLastPosition);
    }
}

void MainWindow::mouseRelease(QMouseEvent *event)
{
    if (isMouseDetached)
    {
        QCursor::setPos(mouseOriginalPosition);
        QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
        isMouseDetached = false;
    }
}

bool MainWindow::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::UpdateRequest:
        tick();
        return true;
    case QEvent::MouseMove:
        mouseMove(dynamic_cast<QMouseEvent*>(event));
        return true;
    case QEvent::MouseButtonPress:
        mousePress(dynamic_cast<QMouseEvent*>(event));
        return true;
    case QEvent::MouseButtonRelease:
        mouseRelease(dynamic_cast<QMouseEvent*>(event));
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

    gl->glEnable(GL_DEPTH_TEST);

    map->renderTick(size.width(), size.height());

    map->dataTick();

    gl->swapBuffers(this);
}
