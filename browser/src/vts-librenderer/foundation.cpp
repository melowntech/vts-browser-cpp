/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "renderer.hpp"

void initializeRenderData();
namespace
{
class RenderDataInitializator
{
public:
    RenderDataInitializator()
    {
        initializeRenderData();
    }
} renderDataInitializatorInstance;
} // namespace

namespace vts { namespace renderer
{

namespace priv
{

uint32 maxAntialiasingSamples = 1;
float maxAnisotropySamples = 0.f;

} // namespace priv

void checkGlImpl(const char *name)
{
    GLint err = glGetError();
    if (err != GL_NO_ERROR)
        log(LogLevel::warn3, std::string("OpenGL error in <") + name + ">");
    switch (err)
    {
    case GL_NO_ERROR:
        return;
    case GL_INVALID_ENUM:
        throw std::runtime_error("gl_invalid_enum");
    case GL_INVALID_VALUE:
        throw std::runtime_error("gl_invalid_value");
    case GL_INVALID_OPERATION:
        throw std::runtime_error("gl_invalid_operation");
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        throw std::runtime_error("gl_invalid_framebuffer_operation");
    case GL_OUT_OF_MEMORY:
        throw std::runtime_error("gl_out_of_memory");
    default:
        throw std::runtime_error("gl_unknown_error");
    }
}

void checkGlFramebuffer(uint32 target)
{
    GLint err = glCheckFramebufferStatus(target);
    switch (err)
    {
    case GL_FRAMEBUFFER_COMPLETE:
        return;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        throw std::runtime_error("gl_framebuffer_incomplete_attachment");
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
        throw std::runtime_error("gl_framebuffer_incomplete_dimensions");
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        throw std::runtime_error("gl_framebuffer_incomplete_missing_attachment");
    case GL_FRAMEBUFFER_UNSUPPORTED:
        throw std::runtime_error("gl_framebuffer_unsupported");
    default:
        throw std::runtime_error("gl_unknown_framebuffer_error");
    }
}

void APIENTRY openglErrorCallback(GLenum source,
                                  GLenum type,
                                  GLenum id,
                                  GLenum severity,
                                  GLsizei,
                                  const GLchar *message,
                                  const void *)
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
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        sevr = "medium";
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

    {
        std::stringstream s;
        s << "OpenGL: <" << id << ">, <" << src << ">, <" << tp
          << ">, <" << sevr << ">, <" << message << ">";
        log(throwing ? LogLevel::err3 : LogLevel::warn3,
                 s.str());
    }

    if (throwing)
    {
        throw std::runtime_error(
                std::string() + "OpenGL: <" + message + ">");
    }
}

void loadGlFunctions(GLADloadproc functionLoader)
{
#ifdef VTSR_OPENGLES
    vts::log(vts::LogLevel::info2,
             "Loading opengl ES function pointers");
    gladLoadGLES2Loader(functionLoader);
#else
    vts::log(vts::LogLevel::info2,
             "Loading opengl (desktop) function pointers");
    gladLoadGLLoader(functionLoader);
#endif
    checkGlImpl("loadGlFunctions");

//#ifndef VTSR_OPENGLES
    if (GLAD_GL_KHR_debug)
        glDebugMessageCallback(&openglErrorCallback, nullptr);

    if (GLAD_GL_EXT_texture_filter_anisotropic)
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropySamples);
    else
        maxAnisotropySamples = 1;

    if (GLAD_GL_EXT_texture_filter_anisotropic)
        glGetIntegerv(GL_MAX_SAMPLES, (GLint*)&maxAntialiasingSamples);
    else
        maxAntialiasingSamples = 0.f;

    checkGlImpl("load gl extensions and attributes");
//#endif

    vts::log(vts::LogLevel::info2, std::string("OpenGL vendor: ")
                                        + (char*)glGetString(GL_VENDOR));
    vts::log(vts::LogLevel::info2, std::string("OpenGL renderer: ")
                                        + (char*)glGetString(GL_RENDERER));
    vts::log(vts::LogLevel::info2, std::string("OpenGL version: ")
                                        + (char*)glGetString(GL_VERSION));
    vts::log(vts::LogLevel::info2, std::string("OpenGL glsl: ")
                        + (char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
}

} } // namespace vts renderer

