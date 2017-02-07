#include "glRenderer.h"
#include "glData.h"

#include "mainwindow.h"
#include <QApplication>

#include <renderer/cache.h>
#include <renderer/map.h>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    MainWindow window;
    window.show();
    GlData glData(&window);
    GlRenderer *glRenderer = window.findChild<GlRenderer*>("rendererContext");

    std::shared_ptr<melown::Cache> cache = melown::newDefaultCache();
    std::shared_ptr<melown::Map> map = std::shared_ptr<melown::Map>(new melown::Map(cache.get(), glRenderer, &glData));

    return application.exec();
}
