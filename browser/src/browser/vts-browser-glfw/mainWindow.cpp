#include <vts/map.hpp>
#include <vts/statistics.hpp>
#include <vts/rendering.hpp>
#include <vts/buffer.hpp>
#include <vts/resources.hpp>
#include "mainWindow.hpp"
#include <GLFW/glfw3.h>

namespace
{

using vts::readInternalMemoryBuffer;

void mousePositionCallback(GLFWwindow *window, double xpos, double ypos)
{
    MainWindow *m = (MainWindow*)glfwGetWindowUserPointer(window);
    m->gui.mousePositionCallback(xpos, ypos);
}

void mouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    MainWindow *m = (MainWindow*)glfwGetWindowUserPointer(window);
    m->gui.mouseScrollCallback(xoffset, yoffset);
}

void keyboardUnicodeCallback(GLFWwindow *window, unsigned int codepoint)
{
    MainWindow *m = (MainWindow*)glfwGetWindowUserPointer(window);
    m->gui.keyboardUnicodeCallback(codepoint);
}

} // namespace

MainWindow::MainWindow() : mousePrevX(0), mousePrevY(0),
    map(nullptr), window(nullptr)
{
    window = glfwCreateWindow(800, 600, "renderer-glfw", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    gladLoadGLLoader((GLADloadproc)&glfwGetProcAddress);
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, &::mousePositionCallback);
    glfwSetScrollCallback(window, &::mouseScrollCallback);
    glfwSetCharCallback(window, &::keyboardUnicodeCallback);
    
    // check for extensions
    anisotropicFilteringAvailable
            = glfwExtensionSupported("GL_EXT_texture_filter_anisotropic");
    
    initializeGpuContext();
    
    { // load shader texture
        shaderTexture = std::make_shared<GpuShader>();
        vts::Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/tex.vert.glsl");
        vts::Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/tex.frag.glsl");
        shaderTexture->loadShaders(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
    }
    
    { // load shader color
        shaderColor = std::make_shared<GpuShader>();
        vts::Buffer vert = readInternalMemoryBuffer(
                    "data/shaders/color.vert.glsl");
        vts::Buffer frag = readInternalMemoryBuffer(
                    "data/shaders/color.frag.glsl");
        shaderColor->loadShaders(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
    }
}

MainWindow::~MainWindow()
{
    if (map)
        map->renderFinalize();
    glfwDestroyWindow(window);
    window = nullptr;
}

void MainWindow::mousePositionCallback(double xpos, double ypos)
{
    vts::Point diff(xpos - mousePrevX, ypos - mousePrevY, 0);
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        map->pan(diff);
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS
        || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
        map->rotate(diff);
    mousePrevX = xpos;
    mousePrevY = ypos;
}

void MainWindow::mouseScrollCallback(double xoffset, double yoffset)
{
    vts::Point diff(0, 0, yoffset * 120);
    map->pan(diff);
}

void MainWindow::keyboardUnicodeCallback(unsigned int codepoint)
{
    // do nothing
}

void MainWindow::drawTexture(vts::DrawTask &t)
{
    shaderTexture->uniformMat4(0, t.mvp);
    shaderTexture->uniformMat3(4, t.uvm);
    shaderTexture->uniform(8, (int)t.externalUv);
    if (t.texMask)
    {
        shaderTexture->uniform(9, 1);
        glActiveTexture(GL_TEXTURE0 + 1);
        dynamic_cast<GpuTextureImpl*>(t.texMask)->bind();
        glActiveTexture(GL_TEXTURE0 + 0);
    }
    else
        shaderTexture->uniform(9, 0);
    dynamic_cast<GpuTextureImpl*>(t.texColor)->bind();
    dynamic_cast<GpuMeshImpl*>(t.mesh)->draw();
}

void MainWindow::drawColor(vts::DrawTask &t)
{
    shaderColor->uniformMat4(0, t.mvp);
    shaderColor->uniformVec3(8, t.color);
    dynamic_cast<GpuMeshImpl*>(t.mesh)->draw();
}

void MainWindow::run()
{
    map->createTexture = std::bind(&MainWindow::createTexture,
                                   this, std::placeholders::_1);
    map->createMesh = std::bind(&MainWindow::createMesh,
                                this, std::placeholders::_1);
    map->renderInitialize();
    gui.initialize(this);
    while (!glfwWindowShouldClose(window))
    {
        double timeFrameStart = glfwGetTime();
        
        checkGl("frame begin");

        glClearColor(0.2, 0.2, 0.2, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        //glCullFace(GL_FRONT);

        int width = 800, height = 600;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        checkGl("frame");
        
        { // draws
            map->renderTick(width, height);
            vts::DrawBatch &draws = map->drawBatch();
            shaderTexture->bind();
            glEnable(GL_CULL_FACE);
            for (vts::DrawTask &t : draws.opaque)
                drawTexture(t);
            glEnable(GL_BLEND);
            for (vts::DrawTask &t : draws.transparent)
                drawTexture(t);
            glDisable(GL_BLEND);
            shaderColor->bind();
            glDisable(GL_CULL_FACE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            for (vts::DrawTask &t : draws.wires)
                drawColor(t);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        checkGl("renderTick");
        
        double timeBeforeSwap = glfwGetTime();
        gui.input();
        gui.render(width, height);
        glfwSwapBuffers(window);
        double timeFrameFinish = glfwGetTime();

        {
            char buffer[500];
            vts::MapStatistics &stat = map->statistics();
            sprintf(buffer, "timing: %3d + %3d = %3d",
                    (int)(1000 * (timeBeforeSwap - timeFrameStart)),
                    (int)(1000 * (timeFrameFinish - timeBeforeSwap)),
                    (int)(1000 * (timeFrameFinish - timeFrameStart))
                    );
            glfwSetWindowTitle(window, buffer);
        }
    }
    gui.finalize();
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
