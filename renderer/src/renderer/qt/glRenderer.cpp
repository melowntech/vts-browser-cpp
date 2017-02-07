#include "glRenderer.h"

using namespace melown;

GlRenderer::GlRenderer(QWidget *parent) : GlContext(parent)
{
}

GlRenderer::~GlRenderer()
{
}

void GlRenderer::initializeGL()
{
    GlContext::initializeGL();
}

void GlRenderer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GlRenderer::paintGL()
{
    glClearColor(qrand() / (float)RAND_MAX, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (map)
        map->render();
}

std::shared_ptr<GlShader> GlRenderer::createShader()
{
    return nullptr;
}

std::shared_ptr<GlTexture> GlRenderer::createTexture()
{
    return nullptr;
}

std::shared_ptr<GlMesh> GlRenderer::createMesh()
{
    return nullptr;
}
