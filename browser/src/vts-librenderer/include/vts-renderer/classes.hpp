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

#ifndef CLASSES_HPP_wefegfuhf
#define CLASSES_HPP_wefegfuhf

#include <string>
#include <vector>

#include <vts-browser/resources.hpp>

#include "foundation.hpp"

namespace vts { namespace renderer
{

class VTSR_API Shader
{
public:
    Shader();
    ~Shader();
    void clear();
    void bind();
    void load(const std::string &vertexShader,
              const std::string &fragmentShader);
    void loadInternal(const std::string &vertexName,
                      const std::string &fragmentName);
    void uniformMat4(uint32 location, const float *value);
    void uniformMat3(uint32 location, const float *value);
    void uniformVec4(uint32 location, const float *value);
    void uniformVec3(uint32 location, const float *value);
    void uniformVec2(uint32 location, const float *value);
    void uniformVec4(uint32 location, const int *value);
    void uniformVec3(uint32 location, const int *value);
    void uniformVec2(uint32 location, const int *value);
    void uniform(uint32 location, float value);
    void uniform(uint32 location, int value);
    uint32 getId() const;

    std::vector<uint32> uniformLocations;
    uint32 loadUniformLocations(const std::vector<const char *> &names);
    void bindTextureLocations(
        const std::vector<std::pair<const char *, uint32>> &binds);
    void bindUniformBlockLocations(
        const std::vector<std::pair<const char *, uint32>> &binds);

    static std::string preamble;

private:
    uint32 id;
};

class VTSR_API Texture
{
public:
    Texture();
    ~Texture();
    void clear();
    void bind();
    void load(ResourceInfo &info, GpuTextureSpec &spec);
    void load(GpuTextureSpec &spec);
    void generateMipmaps();
    uint32 getId() const;
    bool getGrayscale() const;

private:
    uint32 id;
    bool grayscale;
};

class VTSR_API Mesh
{
public:
    Mesh();
    ~Mesh();
    void clear();
    void bind();
    void dispatch();
    void load(ResourceInfo &info, GpuMeshSpec &spec);
    void load(GpuMeshSpec &spec);
    void load(uint32 vao, uint32 vbo, uint32 vio);
    uint32 getVao() const;
    uint32 getVbo() const;
    uint32 getVio() const;

private:
    GpuMeshSpec spec;
    uint32 vao, vbo, vio;
};

class VTSR_API UniformBuffer
{
public:
    UniformBuffer();
    ~UniformBuffer();
    void clear();
    void bind();
    void bindToIndex(uint32 index);
    void load(size_t size, const void *data);
    void load(const Buffer &buffer);

    template <typename T>
    void load(const T &structure)
    {
        load(sizeof(T), &structure);
    }

    uint32 getUbo() const;

private:
    uint32 ubo;
};

} } // namespace vts::renderer

#endif
