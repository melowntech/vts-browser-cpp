#ifndef DATAWINDOW_H_awfevgbhjk
#define DATAWINDOW_H_awfevgbhjk

#include "glContextImpl.h"

#include <QOffscreenSurface>

namespace melown
{
    class Map;
}

class DataWindow : public QOffscreenSurface
{
public:
    DataWindow();
    ~DataWindow();

    void runDataThread();

    Gl *gl;
    melown::Map *map;

private:
    class DataThread *dataThread;
};

#endif
