/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <thread>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <vts-browser/log.hpp>
#include <vts-browser/map.hpp>
#include <vts-browser/mapOptions.hpp>
#include <vts-browser/camera.hpp>
#include <vts-browser/navigation.hpp>
#include <vts-renderer/renderer.hpp>
#include "mainWindow.hpp"
#include "gpuContext.hpp"

static Gl *openglFunctionPointerInstance;

void *openglFunctionPointerProc(const char *name)
{
    return (void*)openglFunctionPointerInstance->getProcAddress(name);
}

MainWindow::MainWindow()
{
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
#ifndef NDEBUG
    format.setOption(QSurfaceFormat::DebugContext);
#endif
    format.setDepthBufferSize(0);
    format.setStencilBufferSize(0);
    setFormat(format);
    setSurfaceType(QWindow::OpenGLSurface);

    resize(QSize(800, 600));
    show();

    gl = std::make_shared<Gl>(this);
    gl->initialize();

    openglFunctionPointerInstance = gl.get();
    vts::renderer::loadGlFunctions(&openglFunctionPointerProc);

    vts::MapCreateOptions mapopts;
    mapopts.clientId = "vts-browser-qt";
    map = std::make_shared<vts::Map>(mapopts);
    map->setMapconfigPath(
            "https://cdn.melown.com/mario/store/melown2015/"
            "map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json",
            "");
    camera = map->createCamera();
    navigation = camera->createNavigation();

    context = std::make_shared<vts::renderer::RenderContext>();
    context->bindLoadFunctions(map.get());
    view = context->createView(camera.get());

    map->renderInitialize();

    dataThread = std::thread(&MainWindow::dataEntry, this);

    lastTime = std::chrono::high_resolution_clock::now();
    requestUpdate();
}

MainWindow::~MainWindow()
{
    if (map)
        map->renderFinalize();
    if (dataThread.joinable())
        dataThread.join();
}

void MainWindow::dataEntry()
{
    vts::setLogThreadName("data");
    gl2 = std::make_shared<Gl>(this);
    gl2->setShareContext(gl.get());
    gl2->initialize();
    vts::renderer::installGlDebugCallback();
    map->dataAllRun();
}

void MainWindow::mouseMove(QMouseEvent *event)
{
    QPoint diff = event->globalPos() - lastMousePosition;
    lastMousePosition = event->globalPos();
    double n[3] = { (double)diff.x(), (double)diff.y(), 0 };
    if (event->buttons() & Qt::LeftButton)
        navigation->pan(n);
    if ((event->buttons() & Qt::RightButton)
        || (event->buttons() & Qt::MiddleButton))
        navigation->rotate(n);
}

void MainWindow::mousePress(QMouseEvent *)
{}

void MainWindow::mouseRelease(QMouseEvent *)
{}

void MainWindow::mouseWheel(QWheelEvent *event)
{
    navigation->zoom(event->angleDelta().y() / 120.0);
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

void MainWindow::tick()
{
    auto currentTime = std::chrono::high_resolution_clock::now();
    double elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(
                lastTime - currentTime).count() * 1e-6;
    lastTime = currentTime;

    requestUpdate();
    if (!isExposed())
        return;

    if (!gl->isValid())
        throw std::runtime_error("invalid gl context");

    gl->makeCurrent(this);
    QSize size = QWindow::size();

    // release build -> catch exceptions and close the window
    // debug build -> let the debugger handle the exceptions
#ifdef NDEBUG
    try
    {
#endif

        map->renderUpdate(elapsedTime);
        camera->setViewportSize(size.width(), size.height());
        camera->renderUpdate();
        view->options().targetFrameBuffer = gl->defaultFramebufferObject();
        view->options().width = size.width();
        view->options().height = size.height();
        view->render();

#ifdef NDEBUG
    }
    catch(...)
    {
        this->close();
        return;
    }
#endif

    // finish the frame
    gl->swapBuffers(this);
}


