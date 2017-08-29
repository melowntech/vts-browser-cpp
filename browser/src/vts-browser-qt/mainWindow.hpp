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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWindow>

namespace vts
{
    class Map;
    class Fetcher;
    class DrawTask;
    class ResourceInfo;
    class GpuTextureSpec;
    class GpuMeshSpec;
}

class Gl;
class GpuShaderImpl;

class MainWindow : public QWindow
{
public:
    MainWindow();
    ~MainWindow();

    bool event(QEvent *event);
    void initialize();
    void tick();

    vts::Map *map;
    std::shared_ptr<Gl> gl;
    std::shared_ptr<vts::Fetcher> fetcher;

private:
    void mouseMove(class QMouseEvent *event);
    void mousePress(class QMouseEvent *event);
    void mouseRelease(class QMouseEvent *event);
    void mouseWheel(class QWheelEvent *event);
    void draw(const vts::DrawTask &task);

    void loadTexture(vts::ResourceInfo &info, const vts::GpuTextureSpec &spec);
    void loadMesh(vts::ResourceInfo &info, const vts::GpuMeshSpec &spec);

    std::shared_ptr<GpuShaderImpl> shaderSurface;

    QPoint mouseLastPosition;
    QPoint mouseOriginalPosition;
    bool isMouseDetached;
};

#endif
