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

#include <unistd.h>
#include <chrono>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>
#include <vts-browser/map.hpp>
#include <vts-browser/draws.hpp>
#include <vts-browser/buffer.hpp>
#include <vts-browser/options.hpp>
#include <vts-browser/credits.hpp>
#include <vts-browser/statistics.hpp>
#include "mainWindow.hpp"
#include "gpuContext.hpp"
#include "fetcher.hpp"

using vts::readInternalMemoryBuffer;

MainWindow::MainWindow() : gl(nullptr), fetcher(nullptr), isMouseDetached(false)
{
    QSurfaceFormat format;
    format.setVersion(4, 4);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    setFormat(format);

    setSurfaceType(QWindow::OpenGLSurface);
    gl = new Gl(this);

    fetcher = std::make_shared<FetcherImpl>();
}

MainWindow::~MainWindow()
{}

void MainWindow::mouseMove(QMouseEvent *event)
{
    QPoint diff = event->globalPos() - mouseLastPosition;
    double n[3] = { (double)diff.x(), (double)diff.y(), 0 };
    if (event->buttons() & Qt::LeftButton)
    { // pan
        map->pan(n);
    }
    if ((event->buttons() & Qt::RightButton)
        || (event->buttons() & Qt::MiddleButton))
    { // rotate
        map->rotate(n);
    }
    mouseLastPosition = event->globalPos();
}

void MainWindow::mousePress(QMouseEvent *)
{}

void MainWindow::mouseRelease(QMouseEvent *)
{}

void MainWindow::mouseWheel(QWheelEvent *event)
{
    map->zoom((double)event->angleDelta().y());
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
    
    { // load shaderTexture
        shaderTexture = std::make_shared<GpuShaderImpl>();
        vts::Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/texture.vert.glsl");
        vts::Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/texture.frag.glsl");
        shaderTexture->loadShaders(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
        std::vector<vts::uint32> &uls = shaderTexture->uniformLocations;
        GpuShaderImpl *s = shaderTexture.get();
        uls.push_back(s->uniformLocation("uniMvp"));
        uls.push_back(s->uniformLocation("uniUvMat"));
        uls.push_back(s->uniformLocation("uniUvMode"));
        uls.push_back(s->uniformLocation("uniMaskMode"));
        uls.push_back(s->uniformLocation("uniTexMode"));
        uls.push_back(s->uniformLocation("uniAlpha"));
        GLuint id = s->programId();
        gl->glUseProgram(id);
        gl->glUniform1i(gl->glGetUniformLocation(id, "texColor"), 0);
        gl->glUniform1i(gl->glGetUniformLocation(id, "texMask"), 1);
    }
    
    { // load shaderColor
        shaderColor = std::make_shared<GpuShaderImpl>();
        vts::Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/color.vert.glsl");
        vts::Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/color.frag.glsl");
        shaderColor->loadShaders(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
        std::vector<vts::uint32> &uls = shaderColor->uniformLocations;
        GpuShaderImpl *s = shaderColor.get();
        uls.push_back(s->uniformLocation("uniMvp"));
        uls.push_back(s->uniformLocation("uniColor"));
    }
}

void MainWindow::draw(const vts::DrawTask &t)
{
    if (t.texColor)
    {
        shaderTexture->bind();
        shaderTexture->uniformMat4(0, t.mvp);
        shaderTexture->uniformMat3(1, t.uvm);
        shaderTexture->uniform(2, (int)t.externalUv);
        if (t.texMask)
        {
            shaderTexture->uniform(3, 1);
            gl->glActiveTexture(GL_TEXTURE0 + 1);
            ((GpuTextureImpl*)t.texMask.get())->bind();
            gl->glActiveTexture(GL_TEXTURE0 + 0);
        }
        else
            shaderTexture->uniform(3, 0);
        GpuTextureImpl *tex = (GpuTextureImpl*)t.texColor.get();
        tex->bind();
        shaderTexture->uniform(4, (int)tex->grayscale);
        shaderTexture->uniform(5, t.color[3]);
    }
    else
    {
        shaderColor->bind();
        shaderColor->uniformMat4(0, t.mvp);
        shaderColor->uniformVec4(1, t.color);
    }
    GpuMeshImpl *m = (GpuMeshImpl*)t.mesh.get();
    m->draw();
}

void MainWindow::tick()
{
    auto timeFrameStart = std::chrono::high_resolution_clock::now();

    requestUpdate();
    if (!isExposed())
        return;

    if (!gl->isValid())
        throw std::runtime_error("invalid gl context");

    map->callbacks().loadTexture = std::bind(&MainWindow::loadTexture, this,
                std::placeholders::_1, std::placeholders::_2);
    map->callbacks().loadMesh = std::bind(&MainWindow::loadMesh, this,
                std::placeholders::_1, std::placeholders::_2);

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
        map->renderTickPrepare();
        map->renderTickRender(size.width(), size.height());
        for (vts::DrawTask &t : map->draws().draws)
            draw(t);

        if ((map->statistics().frameIndex % 120) == 0)
        {
            std::string creditLine = std::string() + "vts-browser-qt: "
                    + map->credits().textShort();
            setTitle(QString::fromUtf8(creditLine.c_str(), creditLine.size()));
        }

#ifdef NDEBUG
    }
    catch(...)
    {
        this->close();
        return;
    }
#endif

    gl->swapBuffers(this);

    auto timeFrameEnd = std::chrono::high_resolution_clock::now();
    auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
            timeFrameEnd - timeFrameStart).count();
    // temporary workaround for when v-sync is missing
    if (frameDuration < 16)
        usleep((16 - frameDuration) * 1000);
}

void MainWindow::loadTexture(vts::ResourceInfo &info,
                             const vts::GpuTextureSpec &spec)
{
    auto r = std::make_shared<GpuTextureImpl>();
    r->loadTexture(info, spec);
    info.userData = r;
}

void MainWindow::loadMesh(vts::ResourceInfo &info,
                          const vts::GpuMeshSpec &spec)
{
    auto r = std::make_shared<GpuMeshImpl>();
    r->loadMesh(info, spec);
    info.userData = r;
}

