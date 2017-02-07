#ifndef GLDATA_H
#define GLDATA_H

#include "glContext.h"

class GlData : public GlContext
{
public:
    GlData(QWidget *parent);
    ~GlData();

    virtual std::shared_ptr<melown::GlShader> createShader();
    virtual std::shared_ptr<melown::GlTexture> createTexture();
    virtual std::shared_ptr<melown::GlMesh> createMesh();

protected:
    virtual void initializeGL();
    virtual void paintGL();
};

#endif // GLDATA_H
