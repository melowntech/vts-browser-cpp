#include <clocale>
#include <QGuiApplication>
#include <QDebug>
#include <melown/map.hpp>
#include "mainWindow.hpp"
#include "gpuContext.hpp"

void usage(char *argv[])
{
    printf("Usage: %s <url>\n", argv[0]);
    abort();
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        usage(argv);

    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication application(argc, argv);

    { // reset locale
        char *locale = std::setlocale(LC_ALL, "C");
        qDebug() << locale;
    }

    MainWindow mainWindow;

    melown::MapFoundation map;
    map.setMapConfig(argv[1]);

    mainWindow.map = &map;
    mainWindow.resize(QSize(800, 600));
    mainWindow.show();
    mainWindow.initialize();
    mainWindow.requestUpdate();

    return application.exec();
}
