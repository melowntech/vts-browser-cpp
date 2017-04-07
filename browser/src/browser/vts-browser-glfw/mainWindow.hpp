#ifndef MAINWINDOW_H_wuiegfzbn
#define MAINWINDOW_H_wuiegfzbn

#include "gpuContext.hpp"

class GLFWwindow;

namespace vts
{

class MapFoundation;
class Resource;
class DrawTask;

} // namespace

class MainWindow
{
public:
    MainWindow();
    ~MainWindow();

    void mousePositionCallback(double xpos, double ypos);
    void mouseScrollCallback(double xoffset, double yoffset);
    void keyboardUnicodeCallback(unsigned int codepoint);

    class Gui
    {
    public:
        void initialize(MainWindow *window);
        void finalize();
        void render(int width, int height);
        void input();
        
        void mousePositionCallback(double xpos, double ypos);
        void mouseScrollCallback(double xoffset, double yoffset);
        void keyboardUnicodeCallback(unsigned int codepoint);
        
        std::shared_ptr<class GuiImpl> impl;
    } gui;
    
    void run();
    
    void drawTexture(vts::DrawTask &t);
    void drawColor(vts::DrawTask &t);
    
    std::shared_ptr<vts::GpuTexture> createTexture
        (const std::string &name);
    std::shared_ptr<vts::GpuMesh> createMesh
        (const std::string &name);

    double mousePrevX, mousePrevY;
    vts::MapFoundation *map;
    GLFWwindow* window;
    
    std::shared_ptr<GpuShader> shaderTexture;
    std::shared_ptr<GpuShader> shaderColor;
};

#endif
