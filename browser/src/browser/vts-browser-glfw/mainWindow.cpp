#include <limits>
#include <cmath>

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

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    MainWindow *m = (MainWindow*)glfwGetWindowUserPointer(window);
    m->gui.mouseButtonCallback(button, action, mods);
}

void mouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    MainWindow *m = (MainWindow*)glfwGetWindowUserPointer(window);
    m->gui.mouseScrollCallback(xoffset, yoffset);
}

void keyboardCallback(GLFWwindow *window, int key, int scancode,
                                            int action, int mods)
{
    MainWindow *m = (MainWindow*)glfwGetWindowUserPointer(window);
    m->gui.keyboardCallback(key, scancode, action, mods);
}

void keyboardUnicodeCallback(GLFWwindow *window, unsigned int codepoint)
{
    MainWindow *m = (MainWindow*)glfwGetWindowUserPointer(window);
    m->gui.keyboardUnicodeCallback(codepoint);
}

} // namespace

namespace vts
{
    class MapImpl;
}

MainWindow::MainWindow() : mousePrevX(0), mousePrevY(0),
    map(nullptr), window(nullptr)
{
    window = glfwCreateWindow(800, 600, "renderer-glfw", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    gladLoadGLLoader((GLADloadproc)&glfwGetProcAddress);
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, &::mousePositionCallback);
    glfwSetMouseButtonCallback(window, &::mouseButtonCallback);
    glfwSetScrollCallback(window, &::mouseScrollCallback);
    glfwSetKeyCallback(window, &::keyboardCallback);
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
    
    { // load mesh mark
        meshMark = std::make_shared<GpuMeshImpl>("mesh_mark");
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer("data/cube.obj"));
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        meshMark->loadMesh(spec);
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
    int mode = 0;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
            mode = 2;
        else
            mode = 1;
    }
    else
    {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS
        || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
            mode = 2;
    }
    switch (mode)
    {
    case 1:
        map->pan(diff);
        break;
    case 2:
        map->rotate(diff);
        break;
    }
    mousePrevX = xpos;
    mousePrevY = ypos;
}

void MainWindow::mouseButtonCallback(int button, int action, int mods)
{
    // do nothing
}

void MainWindow::mouseScrollCallback(double xoffset, double yoffset)
{
    vts::Point diff(0, 0, yoffset * 120);
    map->pan(diff);
}

void MainWindow::keyboardCallback(int key, int scancode, int action, int mods)
{
    // marks
    if (action == GLFW_RELEASE && key == GLFW_KEY_M)
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        y = height - y - 1;
        float depth = std::numeric_limits<float>::quiet_NaN();
        glReadPixels((int)x, (int)y, 1, 1,
                     GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
        depth = depth * 2 - 1;
        x = x / width * 2 - 1;
        y = y / height * 2 - 1;
        Mark mark;
        mark.coord = vts::vec4to3(camViewProj.inverse()
                                  * vts::vec4(x, y, depth, 1), true);
        marks.push_back(mark);
        colorizeMarks();
    }
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
    GpuTextureImpl *tex = dynamic_cast<GpuTextureImpl*>(t.texColor);
    shaderTexture->uniform(10, (int)tex->grayscale);
    tex->bind();
    GpuMeshImpl *m = dynamic_cast<GpuMeshImpl*>(t.mesh);
    m->bind();
    m->dispatch();
}

void MainWindow::drawColor(vts::DrawTask &t)
{
    shaderColor->uniformMat4(0, t.mvp);
    shaderColor->uniformVec3(8, t.color);
    GpuMeshImpl *m = dynamic_cast<GpuMeshImpl*>(t.mesh);
    m->bind();
    m->dispatch();
}

void MainWindow::drawMarks(const Mark &m)
{
    vts::mat4 t = vts::translationMatrix(m.coord)
            * vts::scaleMatrix(map->getPositionViewExtent() * 0.005);
    vts::mat4 mvp = camViewProj * t;
    vts::mat4f mvpf = mvp.cast<float>();
    shaderColor->uniformMat4(0, mvpf.data());
    shaderColor->uniformVec3(8, m.color.data());
    meshMark->dispatch();
}

void MainWindow::run()
{
    map->createTexture = std::bind(&MainWindow::createTexture,
                                   this, std::placeholders::_1);
    map->createMesh = std::bind(&MainWindow::createMesh,
                                this, std::placeholders::_1);
    map->cameraOverrideView = std::bind(&MainWindow::cameraOverrideView,
                                this, std::placeholders::_1);
    map->cameraOverrideProj = std::bind(&MainWindow::cameraOverrideProj,
                                this, std::placeholders::_1);
    map->cameraOverrideFovAspectNearFar = std::bind(
                &MainWindow::cameraOverrideParam, this,
                std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4);
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

        width = 0;
        height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        checkGl("frame");
        
        { // draws
            map->renderTick(width, height); // calls camera overrides
            camViewProj = camProj * camView;
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
            meshMark->bind();
            for (Mark &mark : marks)
                drawMarks(mark);
            glDisable(GL_CULL_FACE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            for (vts::DrawTask &t : draws.wires)
                drawColor(t);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glBindVertexArray(0);
        }
        checkGl("renderTick");
        
        double timeBeforeSwap = glfwGetTime();
        gui.render(width, height);
        gui.input(); // calls glfwPollEvents()
        glfwSwapBuffers(window);
        double timeFrameFinish = glfwGetTime();

        { // timing titlebar
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

void MainWindow::colorizeMarks()
{
    if (marks.empty())
        return;
    float mul = 1.0f / marks.size();
    int index = 0;
    for (Mark &m : marks)
        m.color = vts::convertHsvToRgb(vts::vec3f(index++ * mul, 1, 1));
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

void MainWindow::cameraOverrideParam(double &fov, double &aspect,
                                     double &near, double &far)
{
    camNear = near;
    camFar = far;
}

void MainWindow::cameraOverrideView(double *mat)
{
    for (int i = 0; i < 16; i++)
        camView(i) = mat[i];
}

void MainWindow::cameraOverrideProj(double *mat)
{
    for (int i = 0; i < 16; i++)
        camProj(i) = mat[i];
}
