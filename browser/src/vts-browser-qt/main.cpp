#include <clocale>
#include <QGuiApplication>
#include <QDebug>
#include <vts/map.hpp>
#include <vts/options.hpp>
#include "mainWindow.hpp"
#include "gpuContext.hpp"

void usage(char *argv[])
{
    printf("Usage: %s <url>\n", argv[0]);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        usage(argv);
        return 2;
    }

    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication application(argc, argv);

    // reset locale
    std::setlocale(LC_ALL, "C");

    MainWindow mainWindow;

    vts::MapCreateOptions options;
    vts::Map map(options);
    map.setMapConfigPath(argv[1]);

    mainWindow.map = &map;
    mainWindow.resize(QSize(800, 600));
    mainWindow.show();
    mainWindow.initialize();
    mainWindow.requestUpdate();
    
    return application.exec();
}
