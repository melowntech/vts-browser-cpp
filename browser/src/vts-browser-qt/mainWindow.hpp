#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWindow>
#include "gpuContext.hpp"

namespace vts
{
    class Map;
    class Fetcher;
    class DrawTask;
}

class MainWindow : public QWindow
{
public:
    MainWindow();
    ~MainWindow();

    bool event(QEvent *event);
    void initialize();
    void tick();

    Gl *gl;
    vts::Map *map;
    std::shared_ptr<vts::Fetcher> fetcher;

private:
    void mouseMove(class QMouseEvent *event);
    void mousePress(class QMouseEvent *event);
    void mouseRelease(class QMouseEvent *event);
    void mouseWheel(class QWheelEvent *event);
    void draw(const vts::DrawTask &task);
    
    void loadTexture(vts::ResourceInfo &info, const vts::GpuTextureSpec &spec);
    void loadMesh(vts::ResourceInfo &info, const vts::GpuMeshSpec &spec);
    
    std::shared_ptr<GpuShaderImpl> shaderTexture;
    std::shared_ptr<GpuShaderImpl> shaderColor;
    
    QPoint mouseLastPosition;
    QPoint mouseOriginalPosition;
    bool isMouseDetached;
};

#endif
