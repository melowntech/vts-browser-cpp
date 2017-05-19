#ifndef MAINWINDOW_H_wuiegfzbn
#define MAINWINDOW_H_wuiegfzbn

#include <vector>

#include <vts-browser/math.hpp>
#include <vts-browser/resources.hpp>
#include "gpuContext.hpp"

class GLFWwindow;

namespace vts
{

class Map;
class Resource;
class DrawTask;

} // namespace vts

struct Mark
{
    vts::vec3 coord;
    vts::vec3f color;
    int open;
    
    Mark();
};

class MainWindow
{
public:
    MainWindow();
    ~MainWindow();

    void mousePositionCallback(double xpos, double ypos);
    void mouseButtonCallback(int button, int action, int mods);
    void mouseScrollCallback(double xoffset, double yoffset);
    void keyboardCallback(int key, int scancode, int action, int mods);
    void keyboardUnicodeCallback(unsigned int codepoint);

    class Gui
    {
    public:
        void initialize(MainWindow *window);
        void finalize();
        void render(int width, int height);
        void input();
        
        void mousePositionCallback(double xpos, double ypos);
        void mouseButtonCallback(int button, int action, int mods);
        void mouseScrollCallback(double xoffset, double yoffset);
        void keyboardCallback(int key, int scancode, int action, int mods);
        void keyboardUnicodeCallback(unsigned int codepoint);
        
        std::shared_ptr<class GuiImpl> impl;
    } gui;
    
    void run();
    void colorizeMarks();
    
    void drawVtsTask(vts::DrawTask &t);
    void drawMark(const Mark &m, const Mark *prev);
    
    void loadTexture(vts::ResourceInfo &info, const vts::GpuTextureSpec &spec);
    void loadMesh(vts::ResourceInfo &info, const vts::GpuMeshSpec &spec);
    void cameraOverrideParam(double &fov, double &aspect,
                             double &near, double &far);
    void cameraOverrideView(double *mat);
    void cameraOverrideProj(double *mat);
    
    std::string authPath;
    std::vector<std::string> mapConfigPaths;
    std::shared_ptr<GpuShaderImpl> shaderTexture;
    std::shared_ptr<GpuShaderImpl> shaderColor;
    std::shared_ptr<GpuMeshImpl> meshMark;
    std::shared_ptr<GpuMeshImpl> meshLine;
    std::vector<Mark> marks;
    vts::mat4 camView;
    vts::mat4 camProj;
    vts::mat4 camViewProj;
    double camNear, camFar;
    double mousePrevX, mousePrevY;
    double timingMapProcess;
    double timingAppProcess;
    double timingGuiProcess;
    double timingTotalFrame;
    double timingDataFrame;
    int width, height;
    vts::Map *map;
    GLFWwindow *window;
};

#endif
