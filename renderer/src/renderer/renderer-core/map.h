#ifndef MAP_H_cvukikljqwdf
#define MAP_H_cvukikljqwdf

#include <string>
#include <memory>

namespace melown
{
    class MapImpl
    {
    public:
        MapImpl(const std::string &mapConfigPath);

        std::shared_ptr<class CsConvertor> convertor;
        std::shared_ptr<class Cache> cache;
        std::shared_ptr<class ResourceManager> resources;
        std::shared_ptr<class Renderer> renderer;
        std::string mapConfigPath;
    };
}

#endif
