#include <clocale>

#include "mainWindow.h"
#include "dataWindow.h"
#include "gpuContext.h"
#include "fetcher.h"

#include <QGuiApplication>
#include <QDebug>

#include <renderer/map.h>

/*
#include <QSemaphore>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>

class BarrierData
{
public:
    BarrierData(int count) : count(count) {}

    void wait()
    {
        mutex.lock();
        --count;
        if (count > 0)
            condition.wait(&mutex);
        else
            condition.wakeAll();
        mutex.unlock();
    }

private:
    Q_DISABLE_COPY(BarrierData)
    int count;
    QMutex mutex;
    QWaitCondition condition;
} barrier(2);

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

        window->create();

        barrier.wait();
        barrier.wait();

        window->gl->initialize();
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
*/

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);

    { // set locale
        char *locale = std::setlocale(LC_ALL, "C");
        qDebug() << locale;
    }

    MainWindow mainWindow;

    /*
    DataThread dataThread;
    dataThread.start();
    */

    mainWindow.map = &map;

    /*
    barrier.wait();
    dataThread.window->map = &map;
    dataThread.window->gl->setShareContext(mainWindow.gl);
    barrier.wait();
    */

    {
        FetcherOptions fetcherOptions;
        dynamic_cast<FetcherImpl*>(mainWindow.fetcher)->setOptions(fetcherOptions);
    }

    mainWindow.resize(QSize(800, 600));
    mainWindow.show();
    mainWindow.initialize();
    mainWindow.requestUpdate();

    auto result = application.exec();

    /*
    dataThread.stopping = true;
    dataThread.wait();
    */

    return result;
}
