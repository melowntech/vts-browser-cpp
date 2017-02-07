#ifndef GLCONTEXT_H_awf4564dgfr
#define GLCONTEXT_H_awf4564dgfr

#include <memory>
#include <string>

#include "foundation.h"

namespace melown
{
    class GlShader;
    class GlTexture;
    class GlMesh;

    class MELOWN_API GlContext
    {
    public:
        virtual ~GlContext();

        virtual std::shared_ptr<GlShader> createShader() = 0;
        virtual std::shared_ptr<GlTexture> createTexture() = 0;
        virtual std::shared_ptr<GlMesh> createMesh() = 0;
    };
}

#endif
