#include <cstdio>
#include <glad/glad.h>
#include "gpuContext.h"

namespace
{
    void APIENTRY openglErrorCallback(GLenum source, GLenum type, GLenum id, GLenum severity, GLsizei length, const GLchar *message, const void *user)
    {
        if (id == 131185 && type == GL_DEBUG_TYPE_OTHER)
            return;

        const char *src = nullptr;
        switch (source)
        {
        //case GL_DEBUG_CATEGORY_API_ERROR_AMD:
        case GL_DEBUG_SOURCE_API: src = "api"; break;
        //case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD:
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: src = "window system"; break;
        //case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD:
        case GL_DEBUG_SOURCE_SHADER_COMPILER: src = "shader compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: src = "third party"; break;
        //case GL_DEBUG_CATEGORY_APPLICATION_AMD:
        case GL_DEBUG_SOURCE_APPLICATION: src = "application"; break;
        //case GL_DEBUG_CATEGORY_OTHER_AMD:
        case GL_DEBUG_SOURCE_OTHER: src = "other"; break;
        default: src = "unknown source";
        }

        const char *tp = nullptr;
        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR: tp = "error"; break;
        //case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD:
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: tp = "undefined behavior"; break;
        //case GL_DEBUG_CATEGORY_DEPRECATION_AMD:
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: tp = "deprecated behavior"; break;
        //case GL_DEBUG_TYPE_PORTABILITY_ARB: tp = "portability"; break;
        //case GL_DEBUG_CATEGORY_PERFORMANCE_AMD:
        case GL_DEBUG_TYPE_PERFORMANCE: tp = "performance"; break;
        //case GL_DEBUG_CATEGORY_OTHER_AMD:
        case GL_DEBUG_TYPE_OTHER: tp = "other"; break;
        default: tp = "unknown type";
        }

        const char *sevr = nullptr;
        switch (severity)
        {
        case GL_DEBUG_SEVERITY_HIGH: sevr = "high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM: sevr = "medium"; break;
        case GL_DEBUG_SEVERITY_LOW: sevr = "low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: sevr = "notification"; break;
        default: sevr = "unknown severity";
        }

        fprintf(stderr, "%d %s %s %s\t%s\n", id, src, tp, sevr, message);
    }
}

GpuContext::GpuContext()
{
}

void GpuContext::initialize()
{
    glDebugMessageCallback(&openglErrorCallback, this);
    checkGl("glDebugMessageCallback");
}

void checkGl(const char *name)
{
    GLint err = glGetError();
    switch (err)
    {
#define CAGE_THROW_ERROR(EXC, TXT, ERR) throw TXT;
    case GL_NO_ERROR: return;
    case GL_INVALID_ENUM: CAGE_THROW_ERROR(graphicException, "gl_invalid_enum", err); break;
    case GL_INVALID_VALUE: CAGE_THROW_ERROR(graphicException, "gl_invalid_value", err); break;
    case GL_INVALID_OPERATION: CAGE_THROW_ERROR(graphicException, "gl_invalid_operation", err); break;
    case GL_INVALID_FRAMEBUFFER_OPERATION: CAGE_THROW_ERROR(graphicException, "gl_invalid_framebuffer_operation", err); break;
    case GL_OUT_OF_MEMORY: CAGE_THROW_ERROR(graphicException, "gl_out_of_memory", err); break;
    case GL_STACK_UNDERFLOW: CAGE_THROW_ERROR(graphicException, "gl_stack_underflow", err); break;
    case GL_STACK_OVERFLOW: CAGE_THROW_ERROR(graphicException, "gl_stack_overflow", err); break;
    default: CAGE_THROW_ERROR(graphicException, "gl_unknown_error", err); break;
#undef CAGE_THROW_ERROR
    }
}
