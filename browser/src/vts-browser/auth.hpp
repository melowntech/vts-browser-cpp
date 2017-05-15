#ifndef AUTH_H_sgfuerhgtik
#define AUTH_H_sgfuerhgtik

#include <unordered_set>
#include <string>
#include <memory>

#include <vts/resources.hpp>

namespace vts
{

class AuthJson : public Resource
{
public:
    AuthJson(const std::string &name);
    void load(class MapImpl *base) override;
    void checkTime();
    void authorize(std::shared_ptr<Resource> resource);

private:
    std::string token;
    std::unordered_set<std::string> hostnames;
    uint64 timeValid;
    uint64 timeParsed;
};

} // namespace vts

#endif
