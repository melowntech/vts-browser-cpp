#include <cstdio>
#include <cassert>
#include <cstring>
#include "gpuContext.hpp"

bool anisotropicFilteringAvailable = false;
bool openglDebugAvailable = false;

namespace
{
    void APIENTRY openglErrorCallback(GLenum source,
                                      GLenum type,
                                      GLenum id,
                                      GLenum severity,
                                      GLsizei, // length
                                      const GLchar *message,
                                      const void *) // user
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
            throw std::runtime_error(
                    std::string("opengl error: ") + message);
    }
}

void initializeGpuContext()
{
    if (openglDebugAvailable)
        glDebugMessageCallback(&openglErrorCallback, nullptr);
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

GpuShaderImpl::GpuShaderImpl() : id(0)
{
    uniformLocations.reserve(20);
}

void GpuShaderImpl::clear()
{
    if (id)
        glDeleteProgram(id);
    id = 0;
}

GpuShaderImpl::~GpuShaderImpl()
{
    clear();
}

void GpuShaderImpl::bind()
{
    assert(id > 0);
    glUseProgram(id);
}

int GpuShaderImpl::loadShader(const std::string &source, int stage)
{
    GLuint s = glCreateShader(stage);
    try
    {
        GLchar *src = (GLchar*)source.c_str();
        GLint len = source.length();
        glShaderSource(s, 1, &src, &len);
        glCompileShader(s);
        
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        if (len > 5)
        {
            char *buf = (char*)malloc(len + 1);
            glGetShaderInfoLog(s, len, &len, buf);
            fprintf(stderr, "shader compilation log:\n%s\n\n", buf);
        }
        
        glGetShaderiv(s, GL_COMPILE_STATUS, &len);
        if (len != GL_TRUE)
            throw std::runtime_error("failed to compile shader");
        
        glAttachShader(id, s);
    }
    catch (...)
    {
        glDeleteShader(s);
        throw;
    }
    checkGl("load shader source");
    return s;
}

void GpuShaderImpl::loadShaders(const std::string &vertexShader,
                            const std::string &fragmentShader)
{
    clear();
    id = glCreateProgram();
    try
    {
        GLuint v = loadShader(vertexShader, GL_VERTEX_SHADER);
        GLuint f = loadShader(fragmentShader, GL_FRAGMENT_SHADER);
        glLinkProgram(id);
        glDeleteShader(v);
        glDeleteShader(f);
        
        GLint len = 0;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &len);
        if (len > 5)
        {
            char *buf = (char*)malloc(len + 1);
            glGetProgramInfoLog(id, len, &len, buf);
            fprintf(stderr, "shader link log:\n%s\n\n", buf);
        }
        
        glGetProgramiv(id, GL_LINK_STATUS, &len);
        if (len != GL_TRUE)
            throw std::runtime_error("failed to link shader");
    }
    catch(...)
    {
        glDeleteProgram(id);
        id = 0;
        throw;
    }
    glFinish();
    checkGl("load shader program");
}

void GpuShaderImpl::uniformMat4(vts::uint32 location, const float *value)
{
    glUniformMatrix4fv(uniformLocations[location], 1, GL_FALSE, value);
}

void GpuShaderImpl::uniformMat3(vts::uint32 location, const float *value)
{
    glUniformMatrix3fv(uniformLocations[location], 1, GL_FALSE, value);
}

void GpuShaderImpl::uniformVec4(vts::uint32 location, const float *value)
{
    glUniform4fv(uniformLocations[location], 1, value);
}

void GpuShaderImpl::uniformVec3(vts::uint32 location, const float *value)
{
    glUniform3fv(uniformLocations[location], 1, value);
}

void GpuShaderImpl::uniform(vts::uint32 location, const float value)
{
    glUniform1f(uniformLocations[location], value);
}

void GpuShaderImpl::uniform(vts::uint32 location, const int value)
{
    glUniform1i(uniformLocations[location], value);
}

GpuTextureImpl::GpuTextureImpl(const std::string &name) :
    vts::GpuTexture(name), id(0), grayscale(false)
{}

void GpuTextureImpl::clear()
{
    if (id)
        glDeleteTextures(1, &id);
    id = 0;
}

GpuTextureImpl::~GpuTextureImpl()
{
    clear();
}

void GpuTextureImpl::bind()
{
    assert(id > 0);
    glBindTexture(GL_TEXTURE_2D, id);
}

GLenum GpuTextureImpl::findInternalFormat(const vts::GpuTextureSpec &spec)
{
    switch (spec.components)
    {
    case 1: return GL_R8;
    case 2: return GL_RG8;
    case 3: return GL_RGB8;
    case 4: return GL_RGBA8;
    default:
        throw std::invalid_argument("invalid texture components count");
    }
}

GLenum GpuTextureImpl::findFormat(const vts::GpuTextureSpec &spec)
{
    switch (spec.components)
    {
    case 1: return GL_RED;
    case 2: return GL_RG;
    case 3: return GL_RGB;
    case 4: return GL_RGBA;
    default:
        throw std::invalid_argument("invalid texture components count");
    }
}

void GpuTextureImpl::loadTexture(const vts::GpuTextureSpec &spec)
{
    clear();
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    if (spec.verticalFlip)
    { // vertical flip
        unsigned lineSize = spec.width * spec.components;
        vts::Buffer buffer(lineSize);
        for (unsigned y = 0; y < spec.height / 2; y++)
        {
            char *a = spec.buffer.data() + y * lineSize;
            char *b = spec.buffer.data()
                    + (spec.height - y - 1) * lineSize;
            memcpy(buffer.data(), a, lineSize);
            memcpy(a, b, lineSize);
            memcpy(b, buffer.data(), lineSize);
        }
    }
    glTexImage2D(GL_TEXTURE_2D, 0, findInternalFormat(spec),
                 spec.width, spec.height, 0,
                 findFormat(spec), GL_UNSIGNED_BYTE, spec.buffer.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    if (anisotropicFilteringAvailable)
    {
        float aniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
    }
    
    grayscale = spec.components == 1;
    
    glFinish();
    checkGl("load texture");
    setMemoryUsage(0, spec.buffer.size());
}

GpuMeshImpl::GpuMeshImpl(const std::string &name) : vts::GpuMesh(name),
    vao(0), vbo(0), vio(0)
{}

void GpuMeshImpl::clear()
{
    if (vao)
        glDeleteVertexArrays(1, &vao);
    if (vbo)
        glDeleteBuffers(1, &vbo);
    if (vio)
        glDeleteBuffers(1, &vio);
    vao = vbo = vio = 0;
}

GpuMeshImpl::~GpuMeshImpl()
{
    clear();
}

void GpuMeshImpl::bind()
{
    assert(vbo > 0);
    if (vao)
        glBindVertexArray(vao);
    else
    {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        
        if (vio)
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vio);
        
        for (unsigned i = 0; i < sizeof(vts::GpuMeshSpec::attributes)
             / sizeof(vts::GpuMeshSpec::VertexAttribute); i++)
        {
            vts::GpuMeshSpec::VertexAttribute &a = spec.attributes[i];
            if (a.enable)
            {
                glEnableVertexAttribArray(i);
                glVertexAttribPointer(i, a.components, (GLenum)a.type,
                                      a.normalized ? GL_TRUE : GL_FALSE,
                                      a.stride, (void*)(intptr_t)a.offset);
            }
            else
                glDisableVertexAttribArray(i);
        }
    }
    checkGl("bind mesh");
}

void GpuMeshImpl::dispatch()
{
    if (spec.indicesCount > 0)
        glDrawElements((GLenum)spec.faceMode, spec.indicesCount,
                       GL_UNSIGNED_SHORT, nullptr);
    else
        glDrawArrays((GLenum)spec.faceMode, 0, spec.verticesCount);
    checkGl("dispatch mesh");
}

void GpuMeshImpl::loadMesh(const vts::GpuMeshSpec &spec)
{
    clear();
    this->spec = std::move(spec);
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 spec.vertices.size(), spec.vertices.data(), GL_STATIC_DRAW);
    if (spec.indicesCount)
    {
        glGenBuffers(1, &vio);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vio);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     spec.indices.size(), spec.indices.data(), GL_STATIC_DRAW);
    }
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glFinish();
    checkGl("load mesh");
    setMemoryUsage(sizeof(GpuMeshImpl),
                   spec.vertices.size() + spec.indices.size());
    this->spec.vertices.free();
    this->spec.indices.free();
}
