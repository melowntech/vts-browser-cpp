#include <cstdio>

#include <glad/glad.h>

#include "gpuContext.h"

namespace
{
    void APIENTRY openglErrorCallback(GLenum source,
                                      GLenum type,
                                      GLenum id,
                                      GLenum severity,
                                      GLsizei length,
                                      const GLchar *message,
                                      const void *user)
    {
        if (id == 131185 && type == GL_DEBUG_TYPE_OTHER)
            return;

        bool throwing = false;

        const char *src = nullptr;
        switch (source)
        {
        case GL_DEBUG_SOURCE_API:
            src = "api";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            src = "window system";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            src = "shader compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            src = "third party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            src = "application";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            src = "other";
            break;
        default:
            src = "unknown source";
        }

        const char *tp = nullptr;
        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR:
            tp = "error";
            throwing = true;
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            tp = "undefined behavior";
            throwing = true;
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            tp = "deprecated behavior";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            tp = "performance";
            break;
        case GL_DEBUG_TYPE_OTHER:
            tp = "other";
            break;
        default:
            tp = "unknown type";
        }

        const char *sevr = nullptr;
        switch (severity)
        {
        case GL_DEBUG_SEVERITY_HIGH:
            sevr = "high";
            throwing = true;
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            sevr = "medium";
            throwing = true;
            break;
        case GL_DEBUG_SEVERITY_LOW:
            sevr = "low";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            sevr = "notification";
            break;
        default:
            sevr = "unknown severity";
        }

        fprintf(stderr, "%d %s %s %s\t%s\n", id, src, tp, sevr, message);
        if (throwing)
            throw melown::graphicsException(
                    std::string("opengl problem: ") + message);
    }
}

GpuContext::GpuContext()
{}

void GpuContext::initialize()
{
    glDebugMessageCallback(&openglErrorCallback, this);
    checkGl("glDebugMessageCallback");
}

void checkGl(const char *name)
{
    GLint err = glGetError();
    if (err != GL_NO_ERROR)
        fprintf(stderr, "opengl error in %s\n", name);
    switch (err)
    {
    case GL_NO_ERROR:
        return;
    case GL_INVALID_ENUM:
        throw melown::graphicsException("gl_invalid_enum");
    case GL_INVALID_VALUE:
        throw melown::graphicsException("gl_invalid_value");
    case GL_INVALID_OPERATION:
        throw melown::graphicsException("gl_invalid_operation");
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        throw melown::graphicsException("gl_invalid_framebuffer_operation");
    case GL_OUT_OF_MEMORY:
        throw melown::graphicsException("gl_out_of_memory");
    case GL_STACK_UNDERFLOW:
        throw melown::graphicsException("gl_stack_underflow");
    case GL_STACK_OVERFLOW:
        throw melown::graphicsException("gl_stack_overflow");
    default:
        throw melown::graphicsException("gl_unknown_error");
    }
}

void GpuContext::wiremode(bool wiremode)
{
    if (wiremode)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void GpuContext::activeTextureUnit(melown::uint32 index)
{
    glActiveTexture(GL_TEXTURE0 + index);
}
