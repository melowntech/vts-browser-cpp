#ifndef GPURESOURCES_H_jhsegf
#define GPURESOURCES_H_jhsegf

#include "resource.h"

namespace melown
{
    class MELOWN_API GpuShader : public Resource
    {
    public:
        GpuShader(const std::string &name);

        virtual void bind() = 0;
        virtual void loadShaders(const std::string &vertexShader, const std::string &fragmentShader) = 0;

        void load(class MapImpl *base) override;

        virtual void uniformMat4(uint32 location, const float *value) = 0;
        virtual void uniformMat3(uint32 location, const float *value) = 0;
        virtual void uniform(uint32 location, const float value) = 0;
        virtual void uniform(uint32 location, const int value) = 0;
    };

    class MELOWN_API GpuTextureSpec
    {
    public:
        GpuTextureSpec();

        uint32 width, height;
        uint32 components; // 1, 2, 3 or 4
        uint32 bufferSize; // bytes
        void *buffer;
    };

    class MELOWN_API GpuTexture : public Resource
    {
    public:
        GpuTexture(const std::string &name);

        virtual void bind() = 0;
        virtual void loadTexture(const GpuTextureSpec &spec) = 0;

        void load(class MapImpl *base) override;
    };

    class MELOWN_API GpuMeshSpec
    {
    public:
        enum class FaceMode
        { // OpenGL constants
            Points = 0x0000,
            Lines = 0x0001,
            LineStrip = 0x0003,
            Triangles = 0x0004,
            TriangleStrip = 0x0005,
            TriangleFan = 0x0006,
        };

        struct VertexAttribute
        {
            enum class Type
            { // OpenGL constants
                Float = 0x1406,
            };

            VertexAttribute();
            uint32 offset; // in bytes
            uint32 stride; // in bytes
            uint32 components; // 1, 2, 3 or 4
            Type type;
            bool enable;
            bool normalized;
        };

        GpuMeshSpec();

        VertexAttribute attributes[3];
        const uint16 *indexBufferData;
        void *vertexBufferData;
        uint32 vertexBufferSize; // in bytes
        uint32 verticesCount;
        uint32 indicesCount;
        FaceMode faceMode;
    };

    class MELOWN_API GpuMeshRenderable : public Resource
    {
    public:
        GpuMeshRenderable(const std::string &name);

        virtual void draw() = 0;
        virtual void loadMeshRenderable(const GpuMeshSpec &spec) = 0;

        void load(class MapImpl *base) override;
    };
}

#endif
