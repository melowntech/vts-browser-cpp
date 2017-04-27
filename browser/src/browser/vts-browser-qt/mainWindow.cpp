#include <QGuiApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>
#include <vts/map.hpp>
#include <vts/rendering.hpp>
#include <vts/buffer.hpp>
#include "mainWindow.hpp"
#include "gpuContext.hpp"
#include "fetcher.hpp"

using vts::readInternalMemoryBuffer;

MainWindow::MainWindow() : gl(nullptr), isMouseDetached(false), fetcher(nullptr)
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
    QPoint diff = event->globalPos() - mouseLastPosition;
    if (event->buttons() & Qt::LeftButton)
    { // pan
        vts::Point value(diff.x(), diff.y(), 0);
        map->pan(value);
    }
    if ((event->buttons() & Qt::RightButton)
        || (event->buttons() & Qt::MiddleButton))
    { // rotate
        vts::Point value(diff.x(), diff.y(), 0);
        map->rotate(value);
    }
    mouseLastPosition = event->globalPos();
}

void MainWindow::mousePress(QMouseEvent *event)
{}

void MainWindow::mouseRelease(QMouseEvent *event)
{}

void MainWindow::mouseWheel(QWheelEvent *event)
{
    vts::Point value(0, 0, event->angleDelta().y());
    map->pan(value);
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
    case QEvent::Wheel:
        mouseWheel(dynamic_cast<QWheelEvent*>(event));
        return true;
    default:
        return QWindow::event(event);
    }
}

void MainWindow::initialize()
{
    gl->initialize();
    map->renderInitialize();
    map->dataInitialize(fetcher);
    
    { // load shader
        shader = std::make_shared<GpuShader>();
        vts::Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/texture.vert.glsl");
        vts::Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/texture.frag.glsl");
        shader->loadShaders(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
    }
}

void MainWindow::tick()
{
    requestUpdate();
    if (!isExposed())
        return;

    if (!gl->isValid())
        throw std::runtime_error("invalid gl context");
    
    map->createTexture = std::bind(&MainWindow::createTexture,
                                   this, std::placeholders::_1);
    map->createMesh = std::bind(&MainWindow::createMesh,
                                this, std::placeholders::_1);

    gl->makeCurrent(this);

    QSize size = QWindow::size();
    gl->glViewport(0, 0, size.width(), size.height());
    gl->glClearColor(0.2, 0.2, 0.2, 1);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl->glEnable(GL_DEPTH_TEST);

    map->dataTick();
    map->renderTick(size.width(), size.height());
    
    { // draws
        vts::DrawBatch &draws = map->drawBatch();
        shader->bind();
        for (vts::DrawTask &t : draws.draws)
        {
            if (t.transparent || !t.texColor)
                continue;
            shader->uniformMat4(0, t.mvp);
            shader->uniformMat3(4, t.uvm);
            shader->uniform(8, (int)t.externalUv);
            if (t.texMask)
            {
                shader->uniform(9, 1);
                gl->glActiveTexture(GL_TEXTURE0 + 1);
                dynamic_cast<GpuTextureImpl*>(t.texMask)->bind();
                gl->glActiveTexture(GL_TEXTURE0 + 0);
            }
            else
                shader->uniform(9, 0);
            dynamic_cast<GpuTextureImpl*>(t.texColor)->bind();
            dynamic_cast<GpuMeshImpl*>(t.mesh)->draw();
        }
    }

    gl->swapBuffers(this);
}

std::shared_ptr<vts::GpuTexture> MainWindow::createTexture(
        const std::string &name)
{
    return std::make_shared<GpuTextureImpl>(name);
}

std::shared_ptr<vts::GpuMesh> MainWindow::createMesh(
        const std::string &name)
{
    return std::make_shared<GpuMeshImpl>(name);
}

