#ifndef GLCONTEXT_H_awf4564dgfr
#define GLCONTEXT_H_awf4564dgfr

#include <memory>
#include <string>

#include "foundation.h"

namespace melown
{
    class MELOWN_API GlContext
    {
    public:
        virtual ~GlContext() {}
        virtual std::shared_ptr<class GlShader> createShader() = 0;
        virtual std::shared_ptr<class GlTexture> createTexture() = 0;
        virtual std::shared_ptr<class GlMesh> createMesh() = 0;
    };
}

#endif
