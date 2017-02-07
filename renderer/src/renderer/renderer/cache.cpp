#include <unordered_map>

#include <renderer/cache.h>

namespace melown
{
    Cache::~Cache() {}

    namespace
    {
        class FileImpl
        {
        public:

        };

        class CacheImpl : public Cache
        {
        public:
            CacheImpl(const std::string &hddPath, uint64 hddLimit, uint64 memoryLimit)
            {

            }

            ~CacheImpl()
            {
            }

            virtual void request(const std::string &name)
            {

            }

            virtual void purge(const std::string &name)
            {

            }

            virtual bool available(const std::string &name) const
            {
                return false;
            }

            virtual bool read(const std::string &name, std::shared_ptr<char> &buffer, uint32 &size)
            {
                return false;
            }

            std::unordered_map<std::string, std::unique_ptr<FileImpl>> items;
        };
    }

    const std::shared_ptr<Cache> newDefaultCache(const std::string &hddPath, uint64 hddLimit, uint64 memoryLimit)
    {
        return std::shared_ptr<Cache>(new CacheImpl(hddPath, hddLimit, memoryLimit));
    }
}
