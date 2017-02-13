#include "mainWindow.h"
#include "dataWindow.h"
#include "gpuContext.h"
#include "fetcher.h"

#include <QGuiApplication>
#include <QSemaphore>
#include <QThread>

#include "../renderer/map.h"

QSemaphore dataStartSem;

class DataThread : public QThread
{
public:
    DataThread() :  stopping(false), window(nullptr), fetcher(nullptr)
    {}

    void run()
    {
        window = new DataWindow;
        fetcher = new FetcherImpl;

        {
            FetcherOptions fetcherOptions;
            fetcher->setOptions(fetcherOptions);
        }

        dataStartSem.release();
        window->create();
        window->gl->initialize();
        dataStartSem.acquire();

        window->map->dataInitialize(window->gl, fetcher);
        while(!stopping)
        {
            if (!window->map->dataTick())
                QThread::msleep(10);
            QGuiApplication::processEvents();
        }

        window->map->dataFinalize();
        delete fetcher; fetcher = nullptr;
        delete window; window = nullptr;
    }

    DataWindow *window;
    FetcherImpl *fetcher;
    volatile bool stopping;
};

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);

    MainWindow mainWindow;
    DataThread dataThread;
    dataThread.start();

    mainWindow.map = &map;

    dataStartSem.acquire();
    dataThread.window->map = &map;
    dataStartSem.release();

    mainWindow.resize(QSize(800, 600));
    mainWindow.show();
    dataThread.window->gl->setShareContext(mainWindow.gl);
    mainWindow.initialize();
    mainWindow.requestUpdate();

    auto result = application.exec();

    dataThread.stopping = true;
    dataThread.wait();

    return result;
}
