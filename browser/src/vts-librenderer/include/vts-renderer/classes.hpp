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

namespace privat
{

class VTSR_API ResourceBase : private Immovable
{
#ifndef NDEBUG
public:
    ResourceBase();
    ~ResourceBase();
private:
    uint64 thrId;
#endif
};

} // namespace privat

class VTSR_API Shader : private privat::ResourceBase
{
    std::string debugId;

public:
    Shader();
    ~Shader();
    void setDebugId(const std::string &id);
    void clear();
    void bind();
    void load(const std::string &vertexShader,
              const std::string &fragmentShader);
    void loadInternal(const std::string &vertexName,
                      const std::string &fragmentName);
    void uniformMat4(uint32 location, const float *value, uint32 count = 1);
    void uniformMat3(uint32 location, const float *value, uint32 count = 1);
    void uniformVec4(uint32 location, const float *value, uint32 count = 1);
    void uniformVec3(uint32 location, const float *value, uint32 count = 1);
    void uniformVec2(uint32 location, const float *value, uint32 count = 1);
    void uniformVec4(uint32 location, const int *value, uint32 count = 1);
    void uniformVec3(uint32 location, const int *value, uint32 count = 1);
    void uniformVec2(uint32 location, const int *value, uint32 count = 1);
    void uniform(uint32 location, float value);
    void uniform(uint32 location, int value);
    void uniform(uint32 location, const float *value, uint32 count);
    void uniform(uint32 location, const int *value, uint32 count);
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

    int loadShader(const std::string &source, int stage) const;
};

class VTSR_API Texture : private privat::ResourceBase
{
    std::string debugId;

public:
    Texture();
    ~Texture();
    void setDebugId(const std::string &id);
    void clear();
    void bind();
    void load(ResourceInfo &info, GpuTextureSpec &spec,
            const std::string &debugId);
    void setId(uint32 id);
    uint32 getId() const;
    bool getGrayscale() const;

private:
    uint32 id;
    bool grayscale;
};

class VTSR_API Mesh : private privat::ResourceBase
{
    std::string debugId;

public:
    Mesh();
    ~Mesh();
    void setDebugId(const std::string &id);
    void clear();
    void bind();
    void dispatch();
    void dispatch(uint32 offset, uint32 count); // offset: number of indices/vertices to skip; count: number of indices/vertices to render
    void load(ResourceInfo &info, GpuMeshSpec &spec,
        const std::string &debugId);
    void load(uint32 vao, uint32 vbo, uint32 vio);
    uint32 getVao() const;
    uint32 getVbo() const;
    uint32 getVio() const;

private:
    GpuMeshSpec spec;
    uint32 vao, vbo, vio;
};

class VTSR_API UniformBuffer : private privat::ResourceBase
{
    std::string debugId;

public:
    UniformBuffer();
    ~UniformBuffer();
    UniformBuffer(UniformBuffer &&other);
    UniformBuffer &operator = (UniformBuffer &&other);
    void setDebugId(const std::string &id);
    void clear();
    void bind(); // used for uploading the data
    void bindToIndex(uint32 index); // used for rendering
    void load(const void *data, std::size_t size, uint32 usage);
    void load(const Buffer &buffer, uint32 usage);

    template <class T>
    void load(const T &structure, uint32 usage)
    {
        load(&structure, sizeof(T), usage);
    }

    uint32 getUbo() const;

private:
    std::size_t capacity;
    uint32 ubo;
    uint32 lastUsage;

    void bindInit();
};

} } // namespace vts::renderer

#endif
