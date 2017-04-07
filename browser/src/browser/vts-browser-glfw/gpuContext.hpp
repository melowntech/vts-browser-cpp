#ifndef GPUCONTEXT_H_awfvgbhjk
#define GPUCONTEXT_H_awfvgbhjk

#include <string>
#include <glad/glad.h>
#include <vts/resources.hpp>

extern bool anisotropicFilteringAvailable;

void checkGl(const char *name = nullptr);

void initializeGpuContext();

class GpuShader
{
public:
    GLuint id;

    GpuShader();
    ~GpuShader();
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
};

class GpuTextureImpl : public vts::GpuTexture
{
public:
    GLuint id;

    static GLenum findInternalFormat(const vts::GpuTextureSpec &spec);
    static GLenum findFormat(const vts::GpuTextureSpec &spec);
    
    GpuTextureImpl(const std::string &name);
    ~GpuTextureImpl();
    void clear();
    void bind();
    void loadTexture(const vts::GpuTextureSpec &spec) override;
};

class GpuMeshImpl : public vts::GpuMesh
{
public:
    vts::GpuMeshSpec spec;
    GLuint vao, vbo, vio;

    GpuMeshImpl(const std::string &name);
    ~GpuMeshImpl();
    void clear();
    void draw();
    void loadMesh(const vts::GpuMeshSpec &spec) override;
};

#endif
