#ifndef GPURESOURCES_H_jhsegf
#define GPURESOURCES_H_jhsegf

#include "resource.h"
#include "math.h"

namespace melown
{
    class GpuShader : public Resource
    {
    public:
        virtual void bind() = 0;
        virtual void loadShaders(const std::string &vertexShader, const std::string &fragmentShader) = 0;

        void load(const std::string &name, class Map *base) override;

        virtual void uniform(uint32 location, const mat3 &v) = 0;
        virtual void uniform(uint32 location, const mat4 &v) = 0;
    };

    class GpuTexture : public Resource
    {
    public:
        virtual void bind() = 0;
        virtual void loadTexture(void *buffer, uint32 size) = 0;

        void load(const std::string &name, class Map *base) override;
    };

    class GpuMeshSpec
    {
    public:
        enum class FaceMode
        {
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
            {
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
        const uint16 *indexBuffer;
        void *vertexBuffer;
        uint32 vertexCount;
        uint32 vertexSize;
        uint32 indexCount;
        FaceMode faceMode;
        VertexAttribute attributes[2];
    };

    class GpuMeshRenderable : public Resource
    {
    public:
        virtual void draw() = 0;
        virtual void loadMeshRenderable(const GpuMeshSpec &spec) = 0;

        void load(const std::string &name, class Map *base) override;
    };
}

#endif
