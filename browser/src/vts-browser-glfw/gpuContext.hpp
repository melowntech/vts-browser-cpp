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

#ifndef GPUCONTEXT_H_awfvgbhjk
#define GPUCONTEXT_H_awfvgbhjk

#include <string>
#include <vector>
#include <glad/glad.h>
#include <vts-browser/resources.hpp>

extern bool anisotropicFilteringAvailable;
extern bool openglDebugAvailable;
extern vts::uint32 maxAntialiasingSamples;

void checkGl(const char *name = nullptr);
void checkGlFramebuffer();

void initializeGpuContext();

class GpuShaderImpl
{
public:
    GLuint id;

    GpuShaderImpl();
    ~GpuShaderImpl();
    void clear();
    void bind();
    int loadShader(const std::string &source, int stage);
    void loadShaders(const std::string &vertexShader,
                     const std::string &fragmentShader);
    void uniformMat4(vts::uint32 location, const float *value);
    void uniformMat3(vts::uint32 location, const float *value);
    void uniformVec4(vts::uint32 location, const float *value);
    void uniformVec3(vts::uint32 location, const float *value);
    void uniform(vts::uint32 location, const float value);
    void uniform(vts::uint32 location, const int value);
    
    std::vector<vts::uint32> uniformLocations;
};

class GpuTextureImpl
{
public:
    GLuint id;
    bool grayscale;

    static GLenum findInternalFormat(const vts::GpuTextureSpec &spec);
    static GLenum findFormat(const vts::GpuTextureSpec &spec);
    
    GpuTextureImpl();
    ~GpuTextureImpl();
    void clear();
    void bind();
    void loadTexture(vts::ResourceInfo &info, const vts::GpuTextureSpec &spec);
};

class GpuMeshImpl
{
public:
    vts::GpuMeshSpec spec;
    GLuint vao, vbo, vio;

    GpuMeshImpl();
    ~GpuMeshImpl();
    void clear();
    void bind();
    void dispatch();
    void loadMesh(vts::ResourceInfo &info, const vts::GpuMeshSpec &spec);
};

#endif
