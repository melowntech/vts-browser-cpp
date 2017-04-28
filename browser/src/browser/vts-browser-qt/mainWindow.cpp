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
    
    { // load shaderColor
        shaderColor = std::make_shared<GpuShader>();
        vts::Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/color.vert.glsl");
        vts::Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/color.frag.glsl");
        shaderColor->loadShaders(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
    }
    
    { // load shaderTexture
        shaderTexture = std::make_shared<GpuShader>();
        vts::Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/texture.vert.glsl");
        vts::Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/texture.frag.glsl");
        shaderTexture->loadShaders(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
    }
}

void MainWindow::draw(const vts::DrawTask &t)
{
    if (t.texColor)
    {
        shaderTexture->bind();
        shaderTexture->uniformMat4(0, t.mvp);
        shaderTexture->uniformMat3(4, t.uvm);
        shaderTexture->uniform(8, (int)t.externalUv);
        if (t.texMask)
        {
            shaderTexture->uniform(9, 1);
            gl->glActiveTexture(GL_TEXTURE0 + 1);
            dynamic_cast<GpuTextureImpl*>(t.texMask)->bind();
            gl->glActiveTexture(GL_TEXTURE0 + 0);
        }
        else
            shaderTexture->uniform(9, 0);
        GpuTextureImpl *tex = dynamic_cast<GpuTextureImpl*>(t.texColor);
        tex->bind();
        shaderTexture->uniform(10, (int)tex->grayscale);
        shaderTexture->uniform(11, t.color[3]);
    }
    else
    {
        shaderColor->bind();
        shaderColor->uniformMat4(0, t.mvp);
        shaderColor->uniformVec4(8, t.color);
    }
    GpuMeshImpl *m = dynamic_cast<GpuMeshImpl*>(t.mesh);
    m->draw();
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
    gl->glEnable(GL_BLEND);
    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl->glEnable(GL_DEPTH_TEST);
    gl->glDepthFunc(GL_LEQUAL);
    gl->glEnable(GL_CULL_FACE);
    
    // release build -> catch exceptions and close the window
    // debug build -> let the debugger handle the exceptions
#ifdef NDEBUG
    try
    {
#endif
        
        map->dataTick();
        map->renderTick(size.width(), size.height());
        
        for (vts::DrawTask &t : map->drawBatch().draws)
            draw(t);
        
#ifdef NDEBUG
    }
    catch(...)
    {
        this->close();
    }
#endif

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

