#ifndef GLCONTEXT_H_awf4564dgfr
#define GLCONTEXT_H_awf4564dgfr

#include <memory>
#include <string>

#include "foundation.h"

namespace melown
{
    class Resource;

    class GpuContext
    {
    public:
        virtual ~GpuContext() {}
        virtual std::shared_ptr<Resource> createShader() = 0;
        virtual std::shared_ptr<Resource> createTexture() = 0;
        virtual std::shared_ptr<Resource> createMeshRenderable() = 0;
    };
}

#endif
