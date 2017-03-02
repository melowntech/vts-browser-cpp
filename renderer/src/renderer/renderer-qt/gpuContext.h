#ifndef GLCONTEXTIMPL_H_jkehrbhejkfn
#define GLCONTEXTIMPL_H_jkehrbhejkfn

#include <memory>

#include <QOpenGLContext>
#include <QOpenGLFunctions_4_4_Core>
#include <renderer/gpuContext.h>

class Gl : public QOpenGLContext, public QOpenGLFunctions_4_4_Core,
        public melown::GpuContext
{
public:
    Gl(class QSurface *surface);
    ~Gl();

    std::shared_ptr<melown::Resource> createShader
        (const std::string &name) override;
    std::shared_ptr<melown::Resource> createTexture
        (const std::string &name) override;
    std::shared_ptr<melown::Resource> createMeshRenderable
        (const std::string &name) override;

    void wiremode(bool wiremode) override;

    void initialize();
    void current(bool bind = true);

    void onDebugMessage(const class QOpenGLDebugMessage &message);

    std::shared_ptr<class QOpenGLDebugLogger> logger;

    QSurface *surface;
};

#endif
