
#include <cstdio>
#include <cstdlib>

#include <GLFW/glfw3.h>

#include <renderer/map.h>

#include "mainWindow.h"
#include "dataThread.h"

#include "threadName.h"

void errorCallback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void usage(char *argv[])
{
    printf("Usage: %s <url>\n", argv[0]);
    abort();
}

melown::MapFoundation *globalMap;

int main(int argc, char *argv[])
{
    if (argc != 2)
        usage(argv);

    glfwSetErrorCallback(&errorCallback);
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

    {
        melown::MapFoundation map(argv[1]);
        MainWindow main;
        DataThread data(main.window);
        main.map = &map;
        data.map = &map;
        setThreadName("main");
        main.run();
    }

    glfwTerminate();
    return 0;
}
