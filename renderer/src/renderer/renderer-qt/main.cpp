#include <clocale>

#include "mainWindow.h"
#include "gpuContext.h"
#include "fetcher.h"

#include <QGuiApplication>
#include <QDebug>

#include <renderer/map.h>

void usage(char *argv[])
{
    printf("Usage: %s <url> [username] [password]\n", argv[0]);
    abort();
}

int main(int argc, char *argv[])
{
    FetcherOptions fetcherOptions;

    if (argc < 2 || argc == 3 || argc > 4)
        usage(argv);
    if (argc == 4)
    {
        fetcherOptions.username = argv[2];
        fetcherOptions.password = argv[3];
    }

    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication application(argc, argv);

    { // reset locale
        char *locale = std::setlocale(LC_ALL, "C");
        qDebug() << locale;
    }

    MainWindow mainWindow;
    dynamic_cast<FetcherImpl*>(mainWindow.fetcher)->setOptions(fetcherOptions);

    melown::MapFoundation map(argv[1]);
    //melown::MapFoundation map("https://david.test.mlwn.se/maps/j8.pp.flat/mapConfig.json");
    //melown::MapFoundation map("http://pomerol.internal:8870/Chrudim/mapConfig.json");
    //melown::MapFoundation map("http://cdn.test.mlwn.se/mario/proxy/melown2015/surface/melown/viewfinder-world/mapConfig.json");

    mainWindow.map = &map;
    mainWindow.resize(QSize(800, 600));
    mainWindow.show();
    mainWindow.initialize();
    mainWindow.requestUpdate();

    return application.exec();
}
