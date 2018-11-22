/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <vts-libs/vts/mapconfig.hpp>

#include "credits.hpp"
#include "utilities/json.hpp"

namespace vts
{

namespace
{

static const std::string scopeNames[(int)Credits::Scope::Total_]
    = { "Imagery: ", "Geodata: " };

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

void Credits::tick(CameraCredits &credits)
{
    CameraCredits::Scope *scopes[(int)Scope::Total_] = {
        &credits.imagery, &credits.geodata };
    for (int i = 0; i < (int)Scope::Total_; i++)
    {
        CameraCredits::Scope *s = scopes[i];
        s->credits.clear();
        for (Hit &it : hits[i])
        {
            CameraCredits::Credit c;
            auto t = stor(it.id, std::nothrow);
            if (!t || t->notice.empty())
                continue;
            c.notice = t->notice;
            c.url = t->url ? *t->url : "";
            c.hits = it.hits;
            c.maxLod = it.maxLod;
            s->credits.push_back(c);
        }
        std::sort(s->credits.begin(), s->credits.end(),
                  [](const CameraCredits::Credit &a,
                  const CameraCredits::Credit &b){
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

CameraCredits::CameraCredits()
{}

namespace
{
    Json::Value credit(const CameraCredits::Credit &c)
    {
        Json::Value v;
        v["notice"] = c.notice;
        v["url"] = c.url;
        v["hits"] = c.hits;
        v["maxLod"] = c.maxLod;
        return v;
    }
}

std::string CameraCredits::toJson() const
{
    Json::Value v;
    for (const auto &it : imagery.credits)
        v["imagery"].append(credit(it));
    for (const auto &it : geodata.credits)
        v["geodata"].append(credit(it));
    return jsonToString(v);
}

std::string CameraCredits::textShort() const
{
    std::string result;
    result.reserve(500);
    const Scope *scopes[(int)Credits::Scope::Total_] = { &imagery, &geodata };
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

std::string CameraCredits::textFull() const
{
    std::string result;
    result.reserve(1000);
    const Scope *scopes[(int)Credits::Scope::Total_] = { &imagery, &geodata };
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

void CameraCredits::clear()
{
    Scope *scopes[(int)Credits::Scope::Total_] = { &imagery, &geodata };
    for (int i = 0; i < (int)Credits::Scope::Total_; i++)
    {
        scopes[i]->credits.clear();
    }
}

} // namespace vts
