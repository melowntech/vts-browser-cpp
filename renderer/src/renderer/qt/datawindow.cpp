#include "datawindow.h"
#include "glContextImpl.h"

#include <QThread>

#include <renderer/map.h>

class DataThread : public QThread
{
public:
    DataThread(DataWindow *window) : QThread(window), window(window), destroying(false)
    {}

    ~DataThread()
    {
        destroying = true;
        wait();
    }

    void run() override
    {
        window->gl->initialize();
        window->map->dataInitialize();
        while(!destroying)
        {
            if (!window->map->dataUpdate())
                QThread::msleep(10);
        }
        window->map->dataFinalize();
    }

    DataWindow *window;
    volatile bool destroying;
};

DataWindow::DataWindow() : gl(nullptr), dataThread(nullptr)
{
    gl = new Gl(this);
    dataThread = new DataThread(this);
}

DataWindow::~DataWindow()
{}

void DataWindow::runDataThread()
{
    create();
    gl->moveToThread(dataThread);
    dataThread->start();
}
