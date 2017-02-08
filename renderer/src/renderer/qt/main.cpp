#include "mainwindow.h"
#include "datawindow.h"
#include "glContextImpl.h"
#include "fetcherImpl.h"

#include <QGuiApplication>

#include <renderer/map.h>

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);

    MainWindow main;
    DataWindow data;
    FetcherImpl fetcher;

    melown::MapOptions opt;
    opt.glRenderer = main.gl;
    opt.glData = data.gl;
    opt.fetcher = &fetcher;
    melown::Map map(opt);
    main.map = &map;
    data.map = &map;

    data.runDataThread();

    main.resize(QSize(800, 600));
    main.show();
    main.requestUpdate();

    return application.exec();
}
