#include "mainwindow.h"
#include "datawindow.h"
#include "glContextImpl.h"
#include "fetcherImpl.h"

#include <QGuiApplication>
#include <QDebug>

#include <renderer/map.h>

void fetched(const std::string &name, const char *buffer, melown::uint32 size)
{
    qDebug() << "fetched " << name.data() << ", size: " << size;
    qDebug() << QString::fromUtf8(buffer, size);
}

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);

    MainWindow main;
    DataWindow data;
    FetcherImpl fetcher;
    fetcher.setCallback(&fetched);

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
