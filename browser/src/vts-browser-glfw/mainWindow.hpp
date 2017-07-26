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

struct MapPaths
{
    std::string mapConfig;
    std::string auth;
    std::string sri;
};

struct AppOptions
{
    std::vector<MapPaths> paths;
    std::string initialPosition;
    bool screenshotOnFullRender;
    bool closeOnFullRender;

    AppOptions();
};

class MainWindow
{
public:
    MainWindow(vts::Map *map, const AppOptions &appOptions);
    ~MainWindow();

    void mousePositionCallback(double xpos, double ypos);
    void mouseButtonCallback(int button, int action, int mods);
    void mouseDblClickCallback(int mods);
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
    vts::vec3 getWorldPositionFromCursor();

    void drawVtsTask(vts::DrawTask &t);
    void drawMark(const Mark &m, const Mark *prev);

    void loadTexture(vts::ResourceInfo &info, const vts::GpuTextureSpec &spec);
    void loadMesh(vts::ResourceInfo &info, const vts::GpuMeshSpec &spec);
    void cameraOverrideParam(double &fov, double &aspect,
                             double &near, double &far);
    void cameraOverrideView(double *mat);
    void cameraOverrideProj(double *mat);

    void setMapConfigPath(const MapPaths &paths);

    const AppOptions appOptions;
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
    double dblClickInitTime;
    int width, height;
    int dblClickState;
    vts::Map *const map;
    GLFWwindow *window;
};

#endif
