#ifndef DATATHREAD_H_wefvwehjzg
#define DATATHREAD_H_wefvwehjzg

#include <thread>

class GLFWwindow;

namespace vts
{
class Map;
}

class DataThread
{
public:
    DataThread(GLFWwindow *shared);
    ~DataThread();

    void run();

    vts::Map *map;
    GLFWwindow* window;
    std::thread thr;
    volatile bool stop;
};

#endif
