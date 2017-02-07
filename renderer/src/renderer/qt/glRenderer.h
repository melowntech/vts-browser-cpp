#ifndef GLRENDERER_H
#define GLRENDERER_H

#include "glContext.h"

class GlRenderer : public GlContext
{
public:
    GlRenderer(QWidget *parent);
    ~GlRenderer();

    virtual std::shared_ptr<melown::GlShader> createShader();
    virtual std::shared_ptr<melown::GlTexture> createTexture();
    virtual std::shared_ptr<melown::GlMesh> createMesh();

protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();
};

#endif // GLRENDERER_H
