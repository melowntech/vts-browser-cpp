#include <cstring>
#include <cassert>

#include <glad/glad.h>
#include <renderer/gpuResources.h>

#include "gpuContext.h"

class GpuShaderImpl : public melown::GpuShader
{
public:
    GLuint id;

    GpuShaderImpl(const std::string &name) : melown::GpuShader(name), id(0)
    {}

    void clear()
    {
        if (id)
            glDeleteProgram(id);
        id = 0;
    }

    ~GpuShaderImpl()
    {
        clear();
    }

    void bind() override
    {
        assert(id > 0);
        glUseProgram(id);
    }

    GLuint loadShader(const std::string &source, GLenum stage)
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
                throw melown::graphicsException("failed to compile shader");

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

    void loadShaders(const std::string &vertexShader,
                     const std::string &fragmentShader) override
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
                throw melown::graphicsException("failed to link shader");
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

    void uniformMat4(melown::uint32 location, const float *value) override
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, value);
    }

    void uniformMat3(melown::uint32 location, const float *value) override
    {
        glUniformMatrix3fv(location, 1, GL_FALSE, value);
    }

    void uniformVec4(melown::uint32 location, const float *value) override
    {
        glUniform4fv(location, 1, value);
    }

    void uniformVec3(melown::uint32 location, const float *value) override
    {
        glUniform3fv(location, 1, value);
    }

    void uniform(melown::uint32 location, const float value) override
    {
        glUniform1f(location, value);
    }

    void uniform(melown::uint32 location, const int value) override
    {
        glUniform1i(location, value);
    }
};


class GpuTextureImpl : public melown::GpuTexture
{
public:
    GLuint id;

    GpuTextureImpl(const std::string &name) : melown::GpuTexture(name), id(0)
    {}

    void clear()
    {
        if (id)
            glDeleteTextures(1, &id);
        id = 0;
    }

    ~GpuTextureImpl()
    {
        clear();
    }

    void bind() override
    {
        assert(id > 0);
        glBindTexture(GL_TEXTURE_2D, id);
    }

    static GLenum findInternalFormat(const melown::GpuTextureSpec &spec)
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

    static GLenum findFormat(const melown::GpuTextureSpec &spec)
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

    void loadTexture(const melown::GpuTextureSpec &spec) override
    {
        clear();
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        { // vertical flip
            unsigned lineSize = spec.width * spec.components;
            melown::Buffer buffer(lineSize);
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
        
        glFinish();
        checkGl("load texture");
        gpuMemoryCost = spec.buffer.size();
    }
};


class GpuSubMeshImpl : public melown::GpuMeshRenderable
{
public:
    melown::GpuMeshSpec spec;
    GLuint vao, vbo, vio;

    GpuSubMeshImpl(const std::string &name) : melown::GpuMeshRenderable(name),
        vao(0), vbo(0), vio(0)
    {}

    void clear()
    {
        if (vao)
            glDeleteVertexArrays(1, &vao);
        if (vbo)
            glDeleteBuffers(1, &vbo);
        if (vio)
            glDeleteBuffers(1, &vio);
        vao = vbo = vio = 0;
    }

    ~GpuSubMeshImpl()
    {
        clear();
    }

    void draw() override
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

            for (int i = 0; i < sizeof(melown::GpuMeshSpec::attributes)
                 / sizeof(melown::GpuMeshSpec::VertexAttribute); i++)
            {
                melown::GpuMeshSpec::VertexAttribute &a = spec.attributes[i];
                if (a.enable)
                {
                    glEnableVertexAttribArray(i);
                    glVertexAttribPointer(i, a.components, (GLenum)a.type,
                                          a.normalized ? GL_TRUE : GL_FALSE,
                                          a.stride, (void*)a.offset);
                }
                else
                    glDisableVertexAttribArray(i);
            }
            checkGl("first draw mesh");
        }

        if (spec.indicesCount > 0)
            glDrawElements((GLenum)spec.faceMode, spec.indicesCount,
                           GL_UNSIGNED_SHORT, nullptr);
        else
            glDrawArrays((GLenum)spec.faceMode, 0, spec.verticesCount);
        glBindVertexArray(0);
        checkGl("draw mesh");
    }

    void loadMeshRenderable(const melown::GpuMeshSpec &spec) override
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
        ramMemoryCost = sizeof(GpuSubMeshImpl);
        gpuMemoryCost = spec.vertices.size() + spec.indices.size();
        this->spec.vertices.free();
        this->spec.indices.free();
    }
};

std::shared_ptr<melown::Resource>
GpuContext::createShader(const std::string &name)
{
    return std::shared_ptr<melown::GpuShader>(new GpuShaderImpl(name));
}

std::shared_ptr<melown::Resource>
GpuContext::createTexture(const std::string &name)
{
    return std::shared_ptr<melown::GpuTexture>(new GpuTextureImpl(name));
}

std::shared_ptr<melown::Resource>
GpuContext::createMeshRenderable(const std::string &name)
{
    return std::shared_ptr<melown::GpuMeshRenderable>(new GpuSubMeshImpl(name));
}
