#ifndef GPUCONTEXT_H_awfvgbhjk
#define GPUCONTEXT_H_awfvgbhjk

#include <string>
#include <glad/glad.h>
#include <renderer/resources.h>

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
    void uniformMat4(melown::uint32 location, const float *value);
    void uniformMat3(melown::uint32 location, const float *value);
    void uniformVec4(melown::uint32 location, const float *value);
    void uniformVec3(melown::uint32 location, const float *value);
    void uniform(melown::uint32 location, const float value);
    void uniform(melown::uint32 location, const int value);
};

class GpuTextureImpl : public melown::GpuTexture
{
public:
    GLuint id;

    static GLenum findInternalFormat(const melown::GpuTextureSpec &spec);
    static GLenum findFormat(const melown::GpuTextureSpec &spec);
    
    GpuTextureImpl(const std::string &name);
    ~GpuTextureImpl();
    void clear();
    void bind();
    void loadTexture(const melown::GpuTextureSpec &spec) override;
};

class GpuMeshImpl : public melown::GpuMesh
{
public:
    melown::GpuMeshSpec spec;
    GLuint vao, vbo, vio;

    GpuMeshImpl(const std::string &name);
    ~GpuMeshImpl();
    void clear();
    void draw();
    void loadMesh(const melown::GpuMeshSpec &spec) override;
};

#endif
