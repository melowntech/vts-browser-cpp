#include "mainwindow.h"
#include "datawindow.h"
#include "glContextImpl.h"
#include "fetcherImpl.h"

#include <QGuiApplication>
#include <QDebug>

#include <renderer/map.h>

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);

    MainWindow main;
    DataWindow data;

    FetcherOptions fetcherOptions;
    FetcherImpl fetcher(fetcherOptions);

    data.gl->setShareContext(main.gl);

    melown::MapOptions mapOptions;
    mapOptions.glRenderer = main.gl;
    mapOptions.glData = data.gl;
    mapOptions.fetcher = &fetcher;
    melown::Map map(mapOptions);
    main.map = &map;
    data.map = &map;


    data.runDataThread();

    main.resize(QSize(800, 600));
    main.show();
    main.requestUpdate();

    return application.exec();
}
