
#include <cstdio>
#include <cstdlib>

#include "glad.h"
#include <GLFW/glfw3.h>

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

    while (!glfwWindowShouldClose(window))
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        glClearColor(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
