#ifndef GLCONTEXT_H_awf4564dgfr
#define GLCONTEXT_H_awf4564dgfr

#include <memory>
#include <string>

#include "foundation.h"

namespace melown
{
    class GpuContext
    {
    public:
        virtual ~GpuContext() {}
        virtual std::shared_ptr<class GpuResource> createShader() = 0;
        virtual std::shared_ptr<class GpuResource> createTexture() = 0;
        virtual std::shared_ptr<class GpuResource> createSubMesh() = 0;
        virtual std::shared_ptr<class GpuResource> createMeshAggregate() = 0;
    };
}

#endif
