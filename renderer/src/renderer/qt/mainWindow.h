#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "gpuContext.h"

#include <QWindow>

namespace melown
{
    class Map;
    class Fetcher;
}

class MainWindow : public QWindow
{
public:
    MainWindow();
    ~MainWindow();

    bool event(QEvent *event);
    void initialize();
    void tick();

    Gl *gl;
    melown::Map *map;

    melown::Fetcher *fetcher;
};

#endif
