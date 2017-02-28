#ifndef GPUCONTEXT_H_awfvgbhjk
#define GPUCONTEXT_H_awfvgbhjk

#include <memory>

#include <renderer/gpuContext.h>

void checkGl(const char *name = nullptr);

class GpuContext : public melown::GpuContext
{
public:
    GpuContext();

    std::shared_ptr<melown::Resource> createShader
        (const std::string &name) override;
    std::shared_ptr<melown::Resource> createTexture
        (const std::string &name) override;
    std::shared_ptr<melown::Resource> createMeshRenderable
        (const std::string &name) override;

    void initialize();
};

#endif
