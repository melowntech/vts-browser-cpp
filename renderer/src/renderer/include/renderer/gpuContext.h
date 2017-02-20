#ifndef GLCONTEXT_H_awf4564dgfr
#define GLCONTEXT_H_awf4564dgfr

#include <string>
#include <memory>

#include "foundation.h"

namespace melown
{
    class Resource;

    class MELOWN_API GpuContext
    {
    public:
        virtual ~GpuContext() {}
        virtual std::shared_ptr<Resource> createShader(const std::string &name) = 0;
        virtual std::shared_ptr<Resource> createTexture(const std::string &name) = 0;
        virtual std::shared_ptr<Resource> createMeshRenderable(const std::string &name) = 0;
    };
}

#endif
