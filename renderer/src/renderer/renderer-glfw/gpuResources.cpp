#include <glad/glad.h>

#include "gpuContext.h"

#include <renderer/gpuResources.h>

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
        glUseProgram(id);
    }

    GLuint loadShader(const std::string &source, GLenum stage)
    {
        GLuint s = glCreateShader(stage);
        GLchar *src = (GLchar*)source.c_str();
        GLint len = source.length();
        glShaderSource(s, 1, &src, &len);
        glCompileShader(s);
        glAttachShader(id, s);
        checkGl("load shader source");
        return s;
    }

    void loadShaders(const std::string &vertexShader,
                     const std::string &fragmentShader) override
    {
        clear();
        id = glCreateProgram();
        GLuint v = loadShader(vertexShader, GL_VERTEX_SHADER);
        GLuint f = loadShader(fragmentShader, GL_FRAGMENT_SHADER);
        glLinkProgram(id);
        glDeleteShader(v);
        glDeleteShader(f);
        glFinish();
        checkGl("load shader program");
        state = melown::Resource::State::ready;
    }

    void uniformMat4(melown::uint32 location, const float *value) override
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, value);
    }

    void uniformMat3(melown::uint32 location, const float *value) override
    {
        glUniformMatrix3fv(location, 1, GL_FALSE, value);
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
            throw "invalid texture components count";
        }
    }

    static GLenum findFormat(const melown::GpuTextureSpec &spec)
    {
        switch (spec.components)
        {
        case 1: return GL_R;
        case 2: return GL_RG;
        case 3: return GL_RGB;
        case 4: return GL_RGBA;
        default:
            throw "invalid texture components count";
        }
    }

    void loadTexture(const melown::GpuTextureSpec &spec) override
    {
        clear();
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, findInternalFormat(spec),
                     spec.width, spec.height, 0,
                     findFormat(spec), GL_UNSIGNED_BYTE, spec.buffer);
        glGenerateMipmap(GL_TEXTURE_2D);
        glFinish();
        checkGl("load texture");
        gpuMemoryCost = spec.bufferSize;
        state = melown::Resource::State::ready;
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
        state = melown::Resource::State::initializing;
        clear();
        this->spec = spec;
        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, spec.vertexBufferSize,
                     spec.vertexBufferData, GL_STATIC_DRAW);
        if (spec.indicesCount)
        {
            glGenBuffers(1, &vio);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vio);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         spec.indicesCount * sizeof(spec.indexBufferData[0]),
                    spec.indexBufferData, GL_STATIC_DRAW);
        }
        gpuMemoryCost = spec.vertexBufferSize
                + spec.indicesCount * sizeof(spec.indexBufferData[0]);
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &vao);
        glFinish();
        checkGl("load mesh");
        this->spec.vertexBufferData = nullptr;
        this->spec.indexBufferData = nullptr;
        state = melown::Resource::State::ready;
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
