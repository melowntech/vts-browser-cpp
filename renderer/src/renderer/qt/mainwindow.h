#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "glContextImpl.h"

#include <QWindow>

namespace melown
{
    class Map;
}

class MainWindow : public QWindow
{
public:
    MainWindow();
    ~MainWindow();

    bool event(QEvent *event);
    void tick();

    Gl *gl;
    melown::Map *map;
    bool glInitialized;
};

#endif
