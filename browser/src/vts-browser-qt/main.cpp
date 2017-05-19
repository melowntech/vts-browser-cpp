#include <clocale>
#include <QGuiApplication>
#include <QDebug>
#include <vts-browser/map.hpp>
#include <vts-browser/options.hpp>
#include "mainWindow.hpp"
#include "gpuContext.hpp"

void usage(char *argv[])
{
    printf("Usage: %s <mapconfig-url> [auth-url]\n", argv[0]);
}

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 3)
    {
        usage(argv);
        return 2;
    }

    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication application(argc, argv);

    // reset locale
    std::setlocale(LC_ALL, "C");

    MainWindow mainWindow;

    vts::MapCreateOptions options("vts-browser-qt");
    vts::Map map(options);
    map.setMapConfigPath(argv[1], argc >= 3 ? argv[2] : "");

    mainWindow.map = &map;
    mainWindow.resize(QSize(800, 600));
    mainWindow.show();
    mainWindow.initialize();
    mainWindow.requestUpdate();
    
    return application.exec();
}
