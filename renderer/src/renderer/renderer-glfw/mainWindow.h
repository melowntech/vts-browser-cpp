#ifndef MAINWINDOW_H_wuiegfzbn
#define MAINWINDOW_H_wuiegfzbn

#include "gpuContext.h"

class GLFWwindow;

namespace melown
{

class MapFoundation;
class Resource;


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
    
    std::shared_ptr<melown::GpuTexture> createTexture
        (const std::string &name);
    std::shared_ptr<melown::GpuMesh> createMesh
        (const std::string &name);

    double mousePrevX, mousePrevY;
    melown::MapFoundation *map;
    GLFWwindow* window;
    
    std::shared_ptr<GpuShader> shader;
};

#endif
