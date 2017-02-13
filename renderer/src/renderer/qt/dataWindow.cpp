#include "dataWindow.h"
#include "gpuContext.h"

#include "../renderer/map.h"

DataWindow::DataWindow() : gl(nullptr)
{
    gl = new Gl(this);
}

DataWindow::~DataWindow()
{}
