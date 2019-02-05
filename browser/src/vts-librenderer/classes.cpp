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

namespace vts { namespace renderer
{

#ifdef VTSR_OPENGLES
std::string Shader::preamble = "#version 300 es\n"
#ifdef __APPLE__
        "#extension GL_APPLE_clip_distance : require\n"
#endif
        "precision highp float;\n"
        "precision highp int;\n";
#else
std::string Shader::preamble = "#version 330\n"
        "precision highp float;\n"
        "precision highp int;\n";
#endif

Shader::Shader() : id(0)
{
    uniformLocations.reserve(20);
}

void Shader::clear()
{
    if (id)
        glDeleteProgram(id);
    id = 0;
}

Shader::~Shader()
{
    clear();
}

void Shader::bind()
{
    assert(id > 0);
    glUseProgram(id);
}

namespace
{

int loadShader(const std::string &source, int stage)
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
            Buffer buf(len + 1);
            glGetShaderInfoLog(s, len, &len, buf.data());
            vts::log(vts::LogLevel::err3,
                     std::string("shader compilation log:\n")
                     + std::string(buf.data(), len) + "\n\n");
        }

        glGetShaderiv(s, GL_COMPILE_STATUS, &len);
        if (len != GL_TRUE)
            throw std::runtime_error("failed to compile shader");
    }
    catch (...)
    {
        glDeleteShader(s);
        vts::log(vts::LogLevel::err4,
                 std::string("shader source: \n") + source);
        throw;
    }
    CHECK_GL("load shader source");
    return s;
}

} // namespace

void Shader::load(const std::string &vertexShader,
                  const std::string &fragmentShader)
{
    clear();
    id = glCreateProgram();
    try
    {
        GLuint v = loadShader(preamble + vertexShader, GL_VERTEX_SHADER);
        GLuint f = loadShader(preamble + fragmentShader, GL_FRAGMENT_SHADER);
        glAttachShader(id, v);
        glAttachShader(id, f);
        glLinkProgram(id);
        glDeleteShader(v);
        glDeleteShader(f);

        GLint len = 0;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &len);
        if (len > 5)
        {
            Buffer buf(len + 1);
            glGetProgramInfoLog(id, len, &len, buf.data());
            vts::log(vts::LogLevel::err3,
                     std::string("shader link log:\n")
                     + std::string(buf.data(), len) + "\n\n");
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
    CHECK_GL("load shader program");
}

void Shader::loadInternal(const std::string &vertexName,
                  const std::string &fragmentName)
{
    Buffer vert = readInternalMemoryBuffer(vertexName);
    Buffer frag = readInternalMemoryBuffer(fragmentName);
    load(vert.str(), frag.str());
}

void Shader::uniformMat4(uint32 location, const float *value)
{
    glUniformMatrix4fv(uniformLocations[location], 1, GL_FALSE, value);
}

void Shader::uniformMat3(uint32 location, const float *value)
{
    glUniformMatrix3fv(uniformLocations[location], 1, GL_FALSE, value);
}

void Shader::uniformVec4(uint32 location, const float *value)
{
	static_assert(sizeof(float) == sizeof(GLfloat), "incompatible types");
    glUniform4fv(uniformLocations[location], 1, value);
}

void Shader::uniformVec3(uint32 location, const float *value)
{
    glUniform3fv(uniformLocations[location], 1, value);
}

void Shader::uniformVec2(uint32 location, const float *value)
{
    glUniform2fv(uniformLocations[location], 1, value);
}
    
void Shader::uniformVec4(uint32 location, const int *value)
{
	static_assert(sizeof(int) == sizeof(GLint), "incompatible types");
    glUniform4iv(uniformLocations[location], 1, value);
}

void Shader::uniformVec3(uint32 location, const int *value)
{
    glUniform3iv(uniformLocations[location], 1, value);
}

void Shader::uniformVec2(uint32 location, const int *value)
{
    glUniform2iv(uniformLocations[location], 1, value);
}

void Shader::uniform(uint32 location, const float value)
{
    glUniform1f(uniformLocations[location], value);
}

void Shader::uniform(uint32 location, const int value)
{
    glUniform1i(uniformLocations[location], value);
}

uint32 Shader::getId() const
{
    return id;
}

uint32 Shader::loadUniformLocations(const std::vector<const char *> &names)
{
    bind();
    uint32 res = uniformLocations.size();
    for (auto &it : names)
        uniformLocations.push_back(glGetUniformLocation(id, it));
    return res;
}

void Shader::bindTextureLocations(
    const std::vector<std::pair<const char *, uint32>> &binds)
{
    bind();
    for (auto &it : binds)
        glUniform1i(glGetUniformLocation(id, it.first), it.second);
}

void Shader::bindUniformBlockLocations(
    const std::vector<std::pair<const char *, uint32>> &binds)
{
    for (auto &it : binds)
        glUniformBlockBinding(id, glGetUniformBlockIndex(id, it.first), it.second);
}

Texture::Texture() :
    id(0), grayscale(false)
{}

void Texture::clear()
{
    if (id)
        glDeleteTextures(1, &id);
    id = 0;
}

Texture::~Texture()
{
    clear();
}

void Texture::bind()
{
    assert(id > 0);
    glBindTexture(GL_TEXTURE_2D, id);
}

namespace
{

GLenum findInternalFormat(const GpuTextureSpec &spec)
{
    if (spec.internalFormat)
        return spec.internalFormat;
    switch (spec.type)
    {
    case GpuTypeEnum::Byte:
    case GpuTypeEnum::UnsignedByte:
        switch (spec.components)
        {
        case 1: return GL_R8;
        case 2: return GL_RG8;
        case 3: return GL_RGB8;
        case 4: return GL_RGBA8;
        }
        break;
    case GpuTypeEnum::Short:
    case GpuTypeEnum::UnsignedShort:
        switch (spec.components)
        {
        case 1: return GL_R16;
        case 2: return GL_RG16;
        case 3: return GL_RGB16;
        case 4: return GL_RGBA16;
        }
        break;
    case GpuTypeEnum::Int:
        switch (spec.components)
        {
        case 1: return GL_R32I;
        case 2: return GL_RG32I;
        case 3: return GL_RGB32I;
        case 4: return GL_RGBA32I;
        }
        break;
    case GpuTypeEnum::UnsignedInt:
        switch (spec.components)
        {
        case 1: return GL_R32UI;
        case 2: return GL_RG32UI;
        case 3: return GL_RGB32UI;
        case 4: return GL_RGBA32UI;
        }
        break;
    case GpuTypeEnum::HalfFloat:
        switch (spec.components)
        {
        case 1: return GL_R16F;
        case 2: return GL_RG16F;
        case 3: return GL_RGB16F;
        case 4: return GL_RGBA16F;
        }
        break;
    case GpuTypeEnum::Float:
        switch (spec.components)
        {
        case 1: return GL_R32F;
        case 2: return GL_RG32F;
        case 3: return GL_RGB32F;
        case 4: return GL_RGBA32F;
        }
        break;
    }
    throw std::invalid_argument("cannot deduce texture internal format");
}

GLenum findFormat(const GpuTextureSpec &spec)
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

GpuTextureSpec::FilterMode magFilter(
    const GpuTextureSpec::FilterMode &filterMode)
{
    switch (filterMode)
    {
    case GpuTextureSpec::FilterMode::Nearest:
    case GpuTextureSpec::FilterMode::NearestMipmapNearest:
    case GpuTextureSpec::FilterMode::LinearMipmapNearest:
        return GpuTextureSpec::FilterMode::Nearest;
    case GpuTextureSpec::FilterMode::Linear:
    case GpuTextureSpec::FilterMode::LinearMipmapLinear:
    case GpuTextureSpec::FilterMode::NearestMipmapLinear:
        return GpuTextureSpec::FilterMode::Linear;
    default:
        throw std::invalid_argument("invalid texture filter mode "
            "(in coversion for magnification filter)");
    }
}

} // namespace

void Texture::load(ResourceInfo &info, vts::GpuTextureSpec &spec)
{
    assert(spec.buffer.size() == spec.width * spec.height
           * spec.components * gpuTypeSize(spec.type)
           || spec.buffer.size() == 0);

    clear();
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, findInternalFormat(spec),
                 spec.width, spec.height, 0,
                 findFormat(spec), (GLenum)spec.type, spec.buffer.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        (GLenum)spec.filterMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
        (GLenum)magFilter(spec.filterMode));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
        (GLenum)spec.wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
        (GLenum)spec.wrapMode);

    if (GLAD_GL_EXT_texture_filter_anisotropic)
    {
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropySamples);
    }

    switch (spec.filterMode)
    {
    case GpuTextureSpec::FilterMode::Nearest:
    case GpuTextureSpec::FilterMode::Linear:
        break;
    default:
        glGenerateMipmap(GL_TEXTURE_2D);
        break;
    }

    grayscale = spec.components == 1;

    glFinish();
    CHECK_GL("load texture");
    info.ramMemoryCost += sizeof(*this);
    info.gpuMemoryCost += spec.buffer.size();
}

void Texture::load(vts::GpuTextureSpec &spec)
{
    ResourceInfo info;
    load(info, spec);
}

uint32 Texture::getId() const
{
    return id;
}

bool Texture::getGrayscale() const
{
    return grayscale;
}

void Renderer::loadTexture(ResourceInfo &info, GpuTextureSpec &spec)
{
    auto r = std::make_shared<Texture>();
    r->load(info, spec);
    info.userData = r;
}

Mesh::Mesh() :
    vao(0), vbo(0), vio(0)
{}

void Mesh::clear()
{
    if (vao)
        glDeleteVertexArrays(1, &vao);
    if (vbo)
        glDeleteBuffers(1, &vbo);
    if (vio)
        glDeleteBuffers(1, &vio);
    vao = vbo = vio = 0;
}

Mesh::~Mesh()
{
    clear();
}

void Mesh::bind()
{
    if (vao)
        glBindVertexArray(vao);
    else
    {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        if (vbo)
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            for (unsigned i = 0; i < spec.attributes.size(); i++)
            {
                GpuMeshSpec::VertexAttribute &a = spec.attributes[i];
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

        if (vio)
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vio);
    }
    CHECK_GL("bind mesh");
}

void Mesh::dispatch()
{
    if (spec.indicesCount > 0)
        glDrawElements((GLenum)spec.faceMode, spec.indicesCount,
                       GL_UNSIGNED_SHORT, nullptr);
    else
        glDrawArrays((GLenum)spec.faceMode, 0, spec.verticesCount);
    CHECK_GL("dispatch mesh");
}

void Mesh::load(ResourceInfo &info, GpuMeshSpec &specp)
{
    clear();
    spec = std::move(specp);
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    if (spec.verticesCount)
    {
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                 spec.vertices.size(), spec.vertices.data(), GL_STATIC_DRAW);
    }
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
    CHECK_GL("load mesh");
    info.ramMemoryCost += sizeof(*this);
    info.gpuMemoryCost += spec.vertices.size() + spec.indices.size();
    spec.vertices.free();
    spec.indices.free();
}

void Mesh::load(GpuMeshSpec &spec)
{
    ResourceInfo info;
    load(info, spec);
}

void Mesh::load(uint32 vao, uint32 vbo, uint32 vio)
{
    clear();
    this->vao = vao;
    this->vbo = vbo;
    this->vio = vio;
}

uint32 Mesh::getVao() const
{
    return vao;
}

uint32 Mesh::getVbo() const
{
    return vbo;
}

uint32 Mesh::getVio() const
{
    return vio;
}

void Renderer::loadMesh(ResourceInfo &info, GpuMeshSpec &spec)
{
    auto r = std::make_shared<Mesh>();
    r->load(info, spec);
    info.userData = r;
}

UniformBuffer::UniformBuffer() :
    ubo(0)
{}

UniformBuffer::~UniformBuffer()
{
    clear();
}

void UniformBuffer::clear()
{
    if (ubo)
        glDeleteBuffers(1, &ubo);
    ubo = 0;
}

void UniformBuffer::bind()
{
    if (ubo == 0)
        glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
}

void UniformBuffer::bindToIndex(uint32 index)
{
    if (ubo == 0)
        glGenBuffers(1, &ubo);
    glBindBufferBase(GL_UNIFORM_BUFFER, index, ubo);
}

void UniformBuffer::load(size_t size, const void *data)
{
    assert(ubo != 0);
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
}

void UniformBuffer::load(const Buffer &buffer)
{
    load(buffer.size(), buffer.data());
}

uint32 UniformBuffer::getUbo() const
{
    return ubo;
}

} } // namespace vts renderer

