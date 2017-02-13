#ifndef DATAWINDOW_H_awfevgbhjk
#define DATAWINDOW_H_awfevgbhjk

#include "gpuContext.h"

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

    Gl *gl;
    melown::Map *map;
};

#endif
