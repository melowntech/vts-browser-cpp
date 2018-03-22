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

#include "../map.hpp"
#include "../utilities/json.hpp"

namespace vts
{

GeodataFeatures::GeodataFeatures(vts::MapImpl *map, const std::string &name) :
    Resource(map, name, FetchTask::ResourceType::GeodataFeatures)
{}

void GeodataFeatures::load()
{
    LOG(info2) << "Loading geodata features <" << name << ">";
    data = reply.content.str();
}

GeodataStylesheet::GeodataStylesheet(MapImpl *map, const std::string &name) :
    Resource(map, name, FetchTask::ResourceType::GeodataStylesheet)
{
    priority = std::numeric_limits<float>::infinity();
}

void GeodataStylesheet::load()
{
    LOG(info2) << "Loading geodata stylesheet <" << name << ">";
    data = reply.content.str();
}

GpuGeodata::GpuGeodata(MapImpl *map, const std::string &name)
    : Resource(map, name, FetchTask::ResourceType::General), lod(0)
{
    state = Resource::State::ready;
}

using Json::Value;

namespace
{

template<bool Validating>
struct geoContext
{
    geoContext(GpuGeodata *data, const std::string &style,
               const std::string &features, uint32 lod)
        : data(data),
          style(stringToJson(style)),
          features(stringToJson(features)),
          lod(lod)
    {}

    void process()
    {
        if (Validating)
        {
            // check version
            if (features["version"].asInt() != 1)
            {
                LOGTHROW(err2, std::runtime_error)
                        << "Invalid geodata features <"
                        << data->name << "> version.";
            }
        }

        static const std::vector<std::pair<std::string, std::string>> allTypes
                = { { "point", "points" },
                    { "line", "lines" },
                    { "polygon", "polygons"}
                   };

        const auto allLayerNames = style["layers"].getMemberNames();

        // types
        for (const auto &type : allTypes)
        {
            // groups
            for (const Value &group : features["groups"])
            {
                // features
                for (const Value &feature : group[type.second])
                {
                    current.emplace(type.first, group, feature);
                    // layers
                    for (const std::string &layerName : allLayerNames)
                        processFeature(layerName);
                }
            }
        }
    }

    Value replacement(const std::string &name)
    {
        assert(current);
        if (name.empty())
            return Value();
        switch (name[0])
        {
        case '@': // constant
            return evaluate(style["constants"][name]);
        case '$': // property
            return current->feature["properties"][name.substr(1)];
        case '#': // identifier
            if (name == "#id")
                return current->feature["properties"]["name"];
            if (name == "#group")
                return current->group["id"];
            if (name == "#type")
                return current->type;
            return Value();
        default:
            return name;
        }
    }

    Value evaluate(const Value &expression)
    {
        if (expression.isObject())
        {
            // handle functions
        }
        if (expression.isArray() || expression.isObject())
        {
            Value r(expression);
            for (auto &it : r)
                it = evaluate(it);
            return r;
        }
        if (expression.isString())
        {
            std::string s = expression.asString();
            // find '{'
            auto start = s.find("{");
            if (start != s.npos)
            {
                auto last = start, end = start;
                uint32 cnt = 1;
                while (cnt > 0)
                {
                    end = s.find("}", last + 1);
                    if (Validating)
                    {
                        if (end == s.npos)
                        {
                            LOGTHROW(err1, std::runtime_error)
                                    << "Unmatched <{>";
                        }
                    }
                    auto open = s.find("{", last + 1);
                    if (end > open)
                    {
                        last = open;
                        cnt++;
                    }
                    else
                    {
                        last = end;
                        cnt--;
                    }
                }
                std::string mid = s.substr(start + 1, end - start - 1);
                std::string res = s.substr(0, start - 1)
                        + evaluate(mid).asString()
                        + s.substr(end + 1);
                LOG(info4) << "String <" << s
                           << "> replaced by <" << res << ">";
                return evaluate(res);
            }
            // find '}'
            if (Validating)
            {
                auto end = s.find("}");
                if (end != s.npos)
                {
                    LOGTHROW(err1, std::runtime_error)
                            << "Unmatched <}>";
                }
            }
            // apply replacements
            return replacement(s);
        }
        return expression;
    }

    bool filter(const Value &expression)
    {
        if (!expression.isArray())
            LOGTHROW(err1, std::runtime_error) << "Filter must be array.";

        std::string cond = expression[0].asString();

        if (cond == "skip")
            return false;

        // comparison filters
#define COMP(OP) \
        if (cond == #OP) \
        { \
            if (Validating) \
            { \
                if (expression.size() != 3) \
                    LOGTHROW(err1, std::runtime_error) \
                            << "Invalid filter (" #OP ") array length."; \
            } \
            Value a = evaluate(expression[1]); \
            Value b = evaluate(expression[2]); \
            return a OP b; \
        }
        COMP(==);
        COMP(!=);
        COMP(>=);
        COMP(<=);
        COMP(>);
        COMP(<);
#undef COMP

        // negative filters
        if (!cond.empty() && cond[0] == '!')
        {
            Value v(expression);
            v[0] = cond.substr(1);
            return !filter(v);
        }

        // has filters
        if (cond == "has")
        {
            if (Validating)
            {
                if (expression.size() != 2)
                    LOGTHROW(err1, std::runtime_error)
                            << "Invalid filter (has) array length.";
            }
            Value a = expression[1];
            return a != replacement(a.asString());
        }

        // in filters
        if (cond == "in")
        {
            if (Validating)
            {
                if (expression.size() < 2)
                    LOGTHROW(err1, std::runtime_error)
                            << "Invalid filter (in) array length.";
            }
            std::string v = evaluate(expression[1]).asString();
            for (uint32 i = 2, e = expression.size(); i < e; i++)
            {
                std::string m = evaluate(expression[i]).asString();
                if (v == m)
                    return true;
            }
            return false;
        }

        // aggregate filters
        if (cond == "all")
        {
            for (uint32 i = 1, e = expression.size(); i < e; i++)
                if (!filter(expression[i]))
                    return false;
            return true;
        }
        if (cond == "any")
        {
            for (uint32 i = 1, e = expression.size(); i < e; i++)
                if (filter(expression[i]))
                    return true;
            return false;
        }
        if (cond == "none")
        {
            for (uint32 i = 1, e = expression.size(); i < e; i++)
                if (filter(expression[i]))
                    return false;
            return true;
        }

        // unknown filter
        if (Validating)
        {
            LOGTHROW(err1, std::runtime_error)
                    << "Unknown filter condition type.";
        }
        return false;
    }

    void processFeature(const std::string &layerName,
                        boost::optional<sint32> zOverride
                                = boost::optional<sint32>())
    {
        const Value &layer = style["layers"][layerName];

        if (!zOverride)
        {
            if (!filter(layer["filter"]))
                return;
        }

        data->renders.emplace_back();
    }

    GpuGeodata *const data;
    Value style;
    Value features;
    uint32 lod;

    struct Current
    {
        std::string type;
        const Value &group;
        const Value &feature;
        Current(const std::string &type,
                const Value &group, const Value &feature)
            : type(type), group(group), feature(feature)
        {}
    };

    boost::optional<Current> current;
};

} // namespace

void GpuGeodata::load()
{
    LOG(info2) << "Loading gpu-geodata <" << name << ">";
    renders.clear();

    // this resource is not meant to be downloaded
    assert(reply.content.size() == 0);

    // empty sources
    if (style.empty() || features.empty())
        return;

    // process
    if (map->options.debugValidateGeodataStyles)
    {
        geoContext<true> ctx(this, style, features, lod);
        ctx.process();
    }
    else
    {
        geoContext<false> ctx(this, style, features, lod);
        ctx.process();
    }
}

void GpuGeodata::update(const std::string &s, const std::string &f, uint32 l)
{
    switch ((Resource::State)state)
    {
    case Resource::State::initializing:
        state = Resource::State::ready; // if left in initializing, it would attempt to download it
        // no break
    case Resource::State::errorFatal: // allow reloading when sources change, even if it failed before
    case Resource::State::ready:
        if (style != s || features != f || lod != l)
        {
            style = s;
            features = f;
            lod = l;
            state = Resource::State::downloaded;
        }
        break;
    default:
        // nothing
        break;
    }
}

} // namespace vts
