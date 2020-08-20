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

#include "smoothVariable.hpp"

#include <vts-browser/log.hpp>
#include <vts-browser/math.hpp>
#include <vts-browser/map.hpp>
#include <vts-browser/camera.hpp>
#include <vts-browser/navigation.hpp>
#include <vts-browser/mapOptions.hpp>
#include <vts-browser/cameraOptions.hpp>
#include <vts-browser/navigationOptions.hpp>
#include <vts-browser/resources.hpp>
#include <vts-renderer/renderer.hpp>
#include <vts-renderer/classes.hpp>

namespace vts
{

class Map;

} // namespace vts

struct Mark
{
    vts::vec3 coord = {};
    vts::vec3f color = {};
    int open = false;
};

struct MapPaths
{
    std::string mapConfig;
    std::string auth;
};

struct AppOptions
{
    std::vector<MapPaths> paths;
    std::string initialPosition;
    double guiScale = 1;
    uint32 oversampleRender = 1;
    int renderCompas = 0;
    int simulatedFpsSlowdown = 0;
    bool screenshotOnFullRender = false;
    bool closeOnFullRender = false;
    bool purgeDiskCache = false;
    bool guiVisible = true;
};

void key_callback(struct GLFWwindow *window, int key, int scancode, int action, int mods);
void character_callback(struct GLFWwindow *window, unsigned int codepoint);
void cursor_position_callback(struct GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(struct GLFWwindow *window, int button, int action, int mods);
void scroll_callback(struct GLFWwindow *window, double xoffset, double yoffset);

class MainWindow
{
public:
    MainWindow(struct GLFWwindow *window, vts::Map *map, vts::Camera *camera, vts::Navigation *navigation, const AppOptions &appOptions, const vts::renderer::RenderOptions &renderOptions);
    ~MainWindow();
    void loadResources();

    class Gui
    {
    public:
        void initialize(MainWindow *window);
        void finalize();
        void render(int width, int height);
        void inputBegin();
        void inputEnd();
        void visible(bool visible);
        void scale(double scaling);

        bool key_callback(int key, int scancode, int action, int mods);
        bool character_callback(unsigned int codepoint);
        bool cursor_position_callback(double xpos, double ypos);
        bool mouse_button_callback(int button, int action, int mods);
        bool scroll_callback(double xoffset, double yoffset);
    private:
        std::shared_ptr<class GuiImpl> impl;
    } gui;

    void colorizeMarks();
    vts::vec3 getWorldPositionFromCursor();
    void run();
    void prepareMarks();
    void renderFrame();
    void processEvents();
    void updateWindowSize();
    void makeScreenshot();
    void setMapConfigPath(const MapPaths &paths);

    void key_callback(int key, int scancode, int action, int mods);
    void character_callback(unsigned int codepoint);
    void cursor_position_callback(double xpos, double ypos);
    void mouse_button_callback(int button, int action, int mods);
    void scroll_callback(double xoffset, double yoffset);

    AppOptions appOptions;
    vts::renderer::RenderContext context;
    std::shared_ptr<vts::renderer::RenderView> view;
    std::shared_ptr<vts::renderer::Mesh> meshSphere;
    std::shared_ptr<vts::renderer::Mesh> meshLine;
    std::vector<Mark> marks;
    smoothVariable<double, 60> timingMapSmooth;
    smoothVariable<double, 60> timingFrameSmooth;
    double timingMapProcess = 0;
    double timingAppProcess = 0;
    double timingTotalFrame = 0;
    vts::Map *const map = nullptr;
    vts::Camera *const camera = nullptr;
    vts::Navigation *const navigation = nullptr;
    struct GLFWwindow *window = nullptr;
    double lastXPos = 0;
    double lastYPos = 0;
    double contentScale = 1;
};

#endif
