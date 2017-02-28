#include "mainWindow.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <renderer/map.h>

namespace
{
    void mousePositionCallback(GLFWwindow *window, double xpos, double ypos)
    {
        MainWindow *m = (MainWindow*)glfwGetWindowUserPointer(window);
        m->mousePositionCallback(xpos, ypos);
    }

    void mouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
    {
        MainWindow *m = (MainWindow*)glfwGetWindowUserPointer(window);
        m->mouseScrollCallback(xoffset, yoffset);
    }
}

MainWindow::MainWindow() : mousePrevX(0), mousePrevY(0), map(nullptr), window(nullptr)
{
    window = glfwCreateWindow(800, 600, "renderer-glfw", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    gladLoadGLLoader((GLADloadproc)&glfwGetProcAddress);
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, &::mousePositionCallback);
    glfwSetScrollCallback(window, &::mouseScrollCallback);
    gpu.initialize();
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

void MainWindow::mouseScrollCallback(double, double yoffset)
{
    double diff[3] = {0, 0, yoffset * 120};
    map->pan(diff);
}

void MainWindow::run()
{
    map->renderInitialize(&gpu);
    while (!glfwWindowShouldClose(window))
    {
        checkGl("frame begin");

        glClearColor(0.2, 0.2, 0.2, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        int width = 800, height = 600;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        checkGl("frame");
        map->renderTick(width, height);
        checkGl("renderTick");

        glfwSwapBuffers(window);
        glfwPollEvents();

        checkGl("frame end");
    }
}
