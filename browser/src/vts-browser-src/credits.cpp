#include <vts-libs/vts/mapconfig.hpp>

#include "credits.hpp"

namespace vts
{

namespace
{

static const std::string scopeNames[(int)Credits::Scope::Total_]
    = { "Imagery: ", "Map data: " };

const std::string currentYear()
{
    std::time_t now = std::time(nullptr);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y", std::localtime(&now));
    return buf;
}

const std::string convertNotice(std::string value)
{
    static std::pair<std::string, std::string> replaces[]
            = { { "{copy}", u8"\U000000A9" }, { "{Y}", currentYear() } };
    for (auto &r : replaces)
    {
        while (true)
        {
            auto it = value.find(r.first);
            if (it == value.npos)
                break;
            value = value.substr(0, it) + r.second
                    + value.substr(it + r.first.length());
        }
    }
    return value;
}

} // namespace

boost::optional<vtslibs::registry::CreditId> Credits::find(
        const std::string &name) const
{
    auto r = stor.get(name, std::nothrow);
    if (r)
        return r->numericId;
    return boost::none;
}

void Credits::hit(Scope scope, vtslibs::registry::CreditId id, uint32 lod)
{
    assert(scope < Scope::Total_);
    Hit tmp(id);
    std::vector<Hit> &h = hits[(int)scope];
    auto it = std::lower_bound(h.begin(), h.end(), tmp,
                [](const Hit &a, const Hit &b){
                    return a.id < b.id;
    });
    if (it == h.end() || it->id != tmp.id)
        it = h.insert(it, tmp);
    it->hits++;
    it->maxLod = std::max(it->maxLod, lod);
}

void Credits::tick(MapCredits &credits)
{
    MapCredits::Scope *scopes[(int)Scope::Total_] = {
        &credits.imagery, &credits.data };
    for (int i = 0; i < (int)Scope::Total_; i++)
    {
        MapCredits::Scope *s = scopes[i];
        s->credits.clear();
        for (Hit &it : hits[i])
        {
            MapCredits::Credit c;
            auto t = stor(it.id, std::nothrow);
            if (!t)
                continue;
            c.notice = t->notice;
            c.url = t->url ? *t->url : "";
            c.hits = it.hits;
            c.maxLod = it.maxLod;
            s->credits.push_back(c);
        }
        std::sort(s->credits.begin(), s->credits.end(),
                  [](const MapCredits::Credit &a,
                  const MapCredits::Credit &b){
            return a.hits > b.hits;
        });
        hits[i].clear();
    }
}

void Credits::merge(vtslibs::registry::RegistryBase *reg)
{
    for (auto &it : reg->credits)
        merge(it);
}

void Credits::merge(vtslibs::registry::Credit c)
{
    c.notice = convertNotice(c.notice);
    stor.replace(c);
}

void Credits::purge()
{
    vtslibs::registry::Credit::dict e;
    std::swap(stor, e);
}

Credits::Hit::Hit(vtslibs::registry::CreditId id)
    : id(id), hits(0), maxLod(0)
{}

std::string MapCredits::textShort() const
{
    std::string result;
    result.reserve(200);
    const Scope *scopes[(int)Credits::Scope::Total_] = { &imagery, &data };
    for (int i = 0; i < (int)Credits::Scope::Total_; i++)
    {
        if (scopes[i]->credits.empty())
            continue;
        if (!result.empty())
            result += " | ";
        result += scopeNames[i];
        result += scopes[i]->credits.front().notice;
        if (scopes[i]->credits.size() > 1)
            result += " and others";
    }
    return result;
}

std::string MapCredits::textFull() const
{
    std::string result;
    result.reserve(1000);
    const Scope *scopes[(int)Credits::Scope::Total_] = { &imagery, &data };
    for (int i = 0; i < (int)Credits::Scope::Total_; i++)
    {
        if (scopes[i]->credits.empty())
            continue;
        if (!result.empty())
            result += " | ";
        result += scopeNames[i];
        bool first = true;
        for (auto it : scopes[i]->credits)
        {
            if (first)
                first = false;
            else
                result += ", ";
            result += it.notice;
        }
    }
    return result;
}

} // namespace vts
