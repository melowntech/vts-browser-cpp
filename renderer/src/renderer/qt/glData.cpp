#include "glData.h"

GlData::GlData(QWidget *parent) : GlContext(parent)
{
}

GlData::~GlData()
{
}

void GlData::initializeGL()
{
    GlContext::initializeGL();
}

void GlData::paintGL()
{
    if (map)
        map->data();
}
