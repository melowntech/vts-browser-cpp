
#include <cstdio>
#include <cstdlib>

#include "glad.h"
#include <GLFW/glfw3.h>

#include <renderer/map.h>

#include "gpuContext.h"

void errorCallback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void usage(char *argv[])
{
    printf("Usage: %s <url>\n", argv[0]);
    abort();
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        usage(argv);

    glfwSetErrorCallback(&errorCallback);
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Renderer", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)&glfwGetProcAddress);
    glfwSwapInterval(1);

    melown::MapFoundation map(argv[1]);
    GpuContext gpu;

    map.renderInitialize(&gpu);
    map.dataInitialize(&gpu, nullptr);

    while (!glfwWindowShouldClose(window))
    {
        checkGl("frame begin");

        //glClearColor(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, 0);
        glClearColor(0.2, 0.2, 0.2, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        int width = 800, height = 600;
        glfwGetFramebufferSize(window, &width, &height);

        checkGl("frame");
        map.renderTick(width, height);
        checkGl("renderTick");
        map.dataTick();
        checkGl("dataTick");

        glfwSwapBuffers(window);
        glfwPollEvents();

        checkGl("frame end");
    }

    map.renderFinalize();
    map.dataFinalize();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
