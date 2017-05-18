#ifndef CREDITS_edfgvbbnk
#define CREDITS_edfgvbbnk

#include <vts-libs/registry.hpp>

#include "include/vts-browser/credits.hpp"

namespace vtslibs { namespace vts {

class MapConfig;

}} // namespace vtslibs::vts

namespace vts
{

class Credits
{
public:
    enum class Scope
    {
        Imagery,
        Data,
        
        Total_
    };
    
    boost::optional<vtslibs::registry::CreditId> find(
            const std::string &name) const;
    void hit(Scope scope, vtslibs::registry::CreditId id, uint32 lod);
    void tick(MapCredits &credits);
    void merge(vtslibs::registry::RegistryBase *reg);
    void merge(vtslibs::registry::Credit credit);
    void purge();
    
private:
    vtslibs::registry::Credit::dict stor;
    
    struct Hit
    {
        Hit(vtslibs::registry::CreditId id);
        
        vtslibs::registry::CreditId id;
        uint32 hits;
        uint32 maxLod;
    };
    std::vector<Hit> hits[(int)Scope::Total_];
};

} // namespace vts

#endif
