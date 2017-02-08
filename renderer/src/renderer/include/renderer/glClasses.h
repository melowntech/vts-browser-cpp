#ifndef GLCLASSES_H_jhsegf
#define GLCLASSES_H_jhsegf

#include <math/math_all.hpp>

#include "foundation.h"

namespace melown
{
    class MELOWN_API GlResource
    {
    public:
        virtual ~GlResource() {};
        virtual uint32 memoryCost() const = 0;
        virtual bool ready() const = 0;
    };

    class MELOWN_API GlShader : public GlResource
    {
    public:
        virtual void bind() = 0;
        virtual void loadToGpu(const std::string &vertexShader, const std::string &fragmentShader = "") = 0;
    };

    class MELOWN_API GlTexture : public GlResource
    {
    public:
        virtual void bind() = 0;
        virtual void loadToGpu(void *buffer, uint32 size) = 0;
    };

    class MELOWN_API GlMesh : public GlResource
    {
    public:
        virtual void draw() = 0;
    };
}

#endif
