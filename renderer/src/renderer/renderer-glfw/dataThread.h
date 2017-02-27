#ifndef DATATHREAD_H_wefvwehjzg
#define DATATHREAD_H_wefvwehjzg

#include <thread>
#include "gpuContext.h"
#include "fetcher.h"

class GLFWwindow;

namespace melown
{
    class MapFoundation;
}

class DataThread
{
public:
    DataThread(GLFWwindow *shared);
    ~DataThread();

    void run();

    melown::MapFoundation *map;
    GLFWwindow* window;
    std::shared_ptr<Fetcher> fetcher;
    GpuContext gpu;
    std::thread thr;
    volatile bool stop;
};

#endif
