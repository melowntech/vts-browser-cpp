#ifndef DATATHREAD_H_wefvwehjzg
#define DATATHREAD_H_wefvwehjzg

#include <thread>
#include <vts/fetcher.hpp>

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
    GLFWwindow *window;
    std::shared_ptr<vts::Fetcher> fetcher;
    std::thread thr;
    volatile bool stop;
};

#endif
