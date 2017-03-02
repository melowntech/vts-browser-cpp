#include <clocale>

#include <QGuiApplication>
#include <QDebug>
#include <renderer/map.h>

#include "mainWindow.h"
#include "gpuContext.h"
#include "fetcher.h"

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

    mainWindow.map = &map;
    mainWindow.resize(QSize(800, 600));
    mainWindow.show();
    mainWindow.initialize();
    mainWindow.requestUpdate();

    return application.exec();
}
