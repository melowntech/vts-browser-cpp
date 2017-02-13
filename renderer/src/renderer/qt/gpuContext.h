#ifndef GLCONTEXTIMPL_H_jkehrbhejkfn
#define GLCONTEXTIMPL_H_jkehrbhejkfn

#include <QOpenGLContext>
#include <QOpenGLFunctions_4_4_Core>

#include "../renderer/gpuContext.h"

class Gl : public QOpenGLContext, public QOpenGLFunctions_4_4_Core, public melown::GpuContext
{
public:
    Gl(class QSurface *surface);
    ~Gl();

    std::shared_ptr<melown::GpuResource> createShader() override;
    std::shared_ptr<melown::GpuResource> createTexture() override;
    std::shared_ptr<melown::GpuResource> createSubMesh() override;
    std::shared_ptr<melown::GpuResource> createMeshAggregate() override;

    void initialize();
    void onDebugMessage(const class QOpenGLDebugMessage &message);

    class QOpenGLDebugLogger *logger;

    QSurface *surface;
};

#endif
