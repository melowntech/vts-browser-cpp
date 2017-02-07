#ifndef MAP_H_jihsefk
#define MAP_H_jihsefk

#include <string>

#include "foundation.h"

namespace melown
{
    class MapImpl;
    class Cache;
    class GlContext;

    class MELOWN_API Map
    {
    public:
        Map(Cache *cache, GlContext *glRenderer, GlContext *glData);
        void setMapConfig(const std::string &path);
        void setMapConfig(void *buffer, uint32 length);
        void render();
        void data();

    private:
        MapImpl *impl;
    };
}

#endif
