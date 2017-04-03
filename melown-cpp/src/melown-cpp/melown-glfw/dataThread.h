#ifndef DATATHREAD_H_wefvwehjzg
#define DATATHREAD_H_wefvwehjzg

#include <thread>

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
    std::thread thr;
    volatile bool stop;
};

#endif
