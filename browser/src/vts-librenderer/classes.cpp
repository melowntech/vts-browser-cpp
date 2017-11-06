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

#include <assert.h>

#include "renderer.hpp"

namespace vts { namespace renderer
{

using namespace priv;

#ifdef VTSR_OPENGLES
std::string Shader::preamble = "#version 300 es\n"
        "precision highp float;\n"
        "precision highp int;\n";
#else
std::string Shader::preamble = "#version 330\n";
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
            char *buf = (char*)malloc(len + 1);
            glGetShaderInfoLog(s, len, &len, buf);
            vts::log(vts::LogLevel::err4,
                     std::string("shader compilation log:\n") + buf + "\n\n");
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
    checkGl("load shader source");
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

} // namespace

void Texture::load(ResourceInfo &info, const GpuTextureSpec &spec)
{
    clear();
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, findInternalFormat(spec),
                 spec.width, spec.height, 0,
                 findFormat(spec), GL_UNSIGNED_BYTE, spec.buffer.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    #ifndef VTSR_OPENGLES
    if (GLAD_GL_EXT_texture_filter_anisotropic)
    {
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropySamples);
    }
    #endif

    grayscale = spec.components == 1;

    glFinish();
    checkGl("load texture");
    info.ramMemoryCost += sizeof(*this);
    info.gpuMemoryCost += spec.buffer.size();
}

uint32 Texture::getId() const
{
    return id;
}

bool Texture::getGrayscale() const
{
    return grayscale;
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
    checkGl("bind mesh");
}

void Mesh::dispatch()
{
    if (spec.indicesCount > 0)
        glDrawElements((GLenum)spec.faceMode, spec.indicesCount,
                       GL_UNSIGNED_SHORT, nullptr);
    else
        glDrawArrays((GLenum)spec.faceMode, 0, spec.verticesCount);
    checkGl("dispatch mesh");
}

void Mesh::load(ResourceInfo &info, const GpuMeshSpec &specp)
{
    clear();
    spec = std::move(specp);
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
    info.ramMemoryCost += sizeof(*this);
    info.gpuMemoryCost += spec.vertices.size() + spec.indices.size();
    spec.vertices.free();
    spec.indices.free();
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

} } // namespace vts renderer

