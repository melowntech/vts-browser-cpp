#ifndef GLCONTEXTIMPL_H_jkehrbhejkfn
#define GLCONTEXTIMPL_H_jkehrbhejkfn

#include <QOpenGLContext>
#include <QOpenGLFunctions_4_4_Core>

#include <renderer/glContext.h>

class Gl : public QOpenGLContext, public QOpenGLFunctions_4_4_Core, public melown::GlContext
{
public:
    Gl(class QSurface *surface);
    ~Gl();

    std::shared_ptr<melown::GlShader> createShader() override;
    std::shared_ptr<melown::GlTexture> createTexture() override;
    std::shared_ptr<melown::GlMesh> createMesh() override;

    void initialize();
    void onDebugMessage(const class QOpenGLDebugMessage &message);

    class QOpenGLDebugLogger *logger;

    QSurface *surface;
};

#endif
