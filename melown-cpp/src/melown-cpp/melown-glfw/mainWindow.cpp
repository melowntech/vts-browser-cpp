#include <melown/map.h>
#include <melown/statistics.h>
#include <melown/rendering.h>
#include <melown/buffer.h>
#include <melown/resources.h>
#include "mainWindow.h"
#include <GLFW/glfw3.h>

namespace
{

using melown::readLocalFileBuffer;

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
        melown::Buffer vert = readLocalFileBuffer(
                    "data/shaders/a.vert.glsl");
        melown::Buffer frag = readLocalFileBuffer(
                    "data/shaders/a.frag.glsl");
        shaderTexture->loadShaders(
            std::string(vert.data(), vert.size()),
            std::string(frag.data(), frag.size()));
    }
    
    { // load shader color
        shaderColor = std::make_shared<GpuShader>();
        melown::Buffer vert = readLocalFileBuffer(
                    "data/shaders/color.vert.glsl");
        melown::Buffer frag = readLocalFileBuffer(
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
    double diff[3] = {xpos - mousePrevX, ypos - mousePrevY, 0};
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        map->pan(diff);
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        map->rotate(diff);
    mousePrevX = xpos;
    mousePrevY = ypos;
}

void MainWindow::mouseScrollCallback(double xoffset, double yoffset)
{
    double diff[3] = {0, 0, yoffset * 120};
    map->pan(diff);
}

void MainWindow::keyboardUnicodeCallback(unsigned int codepoint)
{
    // do nothing
}

void MainWindow::drawTexture(melown::DrawTask &t)
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

void MainWindow::drawColor(melown::DrawTask &t)
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
        glEnable(GL_CULL_FACE);
        //glCullFace(GL_FRONT);

        int width = 800, height = 600;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        checkGl("frame");
        
        { // draws
            map->renderTick(width, height);
            melown::DrawBatch &draws = map->drawBatch();
            shaderTexture->bind();
            for (melown::DrawTask &t : draws.opaque)
                drawTexture(t);
            glEnable(GL_BLEND);
            for (melown::DrawTask &t : draws.transparent)
                drawTexture(t);
            glDisable(GL_BLEND);
            shaderColor->bind();
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            for (melown::DrawTask &t : draws.wires)
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
            melown::MapStatistics &stat = map->statistics();
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

std::shared_ptr<melown::GpuTexture> MainWindow::createTexture(
        const std::string &name)
{
    return std::make_shared<GpuTextureImpl>(name);
}

std::shared_ptr<melown::GpuMesh> MainWindow::createMesh(
        const std::string &name)
{
    return std::make_shared<GpuMeshImpl>(name);
}
