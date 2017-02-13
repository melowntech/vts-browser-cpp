#ifndef RENDERER_H_wekojgib
#define RENDERER_H_wekojgib

namespace melown
{
    class Renderer
    {
    public:
        static Renderer *create(class Map *map);

        Renderer();
        virtual ~Renderer();

        virtual void renderInitialize() = 0;
        virtual void renderTick() = 0;
        virtual void renderFinalize() = 0;
    };
}

#endif
