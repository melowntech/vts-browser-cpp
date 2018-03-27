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
    enum class Type
    {
        Point,
        Line,
        Polygon,
    };

    typedef std::array<uint16, 3> Point;

    Point convertPoint(const Value &v)
    {
        if (Validating)
        {
            for (uint32 i = 0; i < 3; i++)
            {
                if (v[i].asUInt() > 65535)
                {
                    LOGTHROW(err1, std::runtime_error)
                            << "Point index outside range.";
                }
            }
        }
        return { (uint16)v[0].asUInt(),
                 (uint16)v[1].asUInt(),
                 (uint16)v[2].asUInt() };
    }

    vec4f convertColor(const Value &v)
    {
        return vec4f(v[0].asInt(), v[1].asInt(), v[2].asInt(), v[3].asInt())
                / 255.f;
    }

    geoContext(GpuGeodata *data, const std::string &style,
               const std::string &features, uint32 lod)
        : data(data),
          style(stringToJson(style)),
          features(stringToJson(features)),
          lod(lod)
    {}

    void solveInheritance()
    {
        // todo
    }

    bool isLayerStyleRequired(const Value &v)
    {
        if (v.isNull())
            return false;
        if (v.isConvertibleTo(Json::ValueType::booleanValue))
            return v.asBool();
        return true;
    }

    std::vector<std::string> filterLayersByType(Type t)
    {
        std::vector<std::string> result;
        const auto allLayerNames = style["layers"].getMemberNames();
        for (const std::string &layerName : allLayerNames)
        {
            Value layer = style["layers"][layerName];
            if (layer.isMember("filter") && layer["filter"].isArray()
                    && layer["filter"][0] == "skip")
                continue;
            bool line = isLayerStyleRequired(layer["line"]);
            bool point = isLayerStyleRequired(layer["point"]);
            bool icon = isLayerStyleRequired(layer["icon"]);
            bool label = isLayerStyleRequired(layer["label"]);
            bool polygon = isLayerStyleRequired(layer["polygon"]);
            bool ok = false;
            switch (t)
            {
            case Type::Point:
                ok = point || icon || label;
                break;
            case Type::Line:
                ok = point || line;
                break;
            case Type::Polygon:
                ok = point || line || polygon;
                break;
            }
            if (ok)
                result.push_back(layerName);
        }
        return result;
    }

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

        solveInheritance();

        static const std::vector<std::pair<Type, std::string>> allTypes
                = { { Type::Point, "points" },
                    { Type::Line, "lines" },
                    { Type::Polygon, "polygons"}
                   };

        // style layers filtered by valid feature types
        std::map<Type, std::vector<std::string>> typedLayerNames;
        for (Type t : { Type::Point, Type::Line, Type::Polygon })
            typedLayerNames[t] = filterLayersByType(t);

        // groups
        for (const Value &group : features["groups"])
        {
            this->group.emplace(group);
            // types
            for (const auto &type : allTypes)
            {
                this->type.emplace(type.first);
                const auto &layers = typedLayerNames[type.first];
                if (layers.empty())
                    continue;
                // features
                for (const Value &feature : group[type.second])
                {
                    this->feature.emplace(feature);
                    // layers
                    for (const std::string &layerName : layers)
                        processFeature(layerName);
                }
            }
            this->type.reset();
            this->feature.reset();
            finishGroup();
        }
    }

    Value replacement(const std::string &name)
    {
        assert(group);
        assert(type);
        assert(feature);
        if (name.empty())
            return Value();
        switch (name[0])
        {
        case '@': // constant
            return evaluate(style["constants"][name]);
        case '$': // property
            return feature->properties[name.substr(1)];
        case '#': // identifier
            if (name == "#id")
                return feature->properties["name"];
            if (name == "#group")
                return group->group["id"];
            if (name == "#type")
            {
                switch (*type)
                {
                case Type::Point:
                    return "point";
                case Type::Line:
                    return "line";
                case Type::Polygon:
                    return "polygon";
                }
            }
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
        if (Validating)
        {
            if (!expression.isArray())
                LOGTHROW(err1, std::runtime_error) << "Filter must be array.";
        }

        std::string cond = expression[0].asString();

        if (cond == "skip")
        {
            if (Validating)
            {
                if (expression.size() != 1)
                    LOGTHROW(err1, std::runtime_error)
                            << "Invalid filter (skip) array length.";
            }
            return false;
        }

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

        // filter
        if (!zOverride)
        {
            if (!filter(layer["filter"]))
                return;
        }

        // inherit

        // visible
        //if (layer.isMember("visible") && !evaluate(layer["visible"]).asBool())
        //    return;

        // z-index

        // zbuffer-offset

        // visibility

        // culling

        // line
        if (evaluate(layer["line"]).asBool())
        {
            vec4f color = convertColor(layer["line-color"]);
            switch (*type)
            {
            case Type::Point:
                break;
            case Type::Line:
            {
                const Value &la = feature->feature["lines"];
                for (const Value &l : la)
                {
                    Point last = {};
                    bool second = false;
                    for (const Value &pv : l)
                    {
                        Point p = convertPoint(pv);
                        if (second)
                        {
                            LineMutable lm;
                            lm.p[0] = last;
                            lm.p[1] = p;
                            LineImmutable li;
                            li.color = color;
                            cacheLines[li].push_back(lm);
                        }
                        else
                            second = true;
                        last = p;
                    }
                }
            } break;
            case Type::Polygon:
            {
                // todo
            } break;
            }
        }

        // line-label

        // point

        // icon

        // label

        // polygon

        // next-pass
    }

    void finishGroup()
    {
        // this function takes all the cached values,
        //   merges them as much as possible
        //   and generates actual gpu meshes and render tasks

        finishGroupLines();
        // todo
    }

    void finishGroupLines()
    {
        for (const std::pair<const LineImmutable,
                std::vector<LineMutable>> &lines : cacheLines)
        {
            GpuMeshSpec spec;
            spec.verticesCount = lines.second.size() * 2;
            spec.vertices.allocate(lines.second.size() * sizeof(Point) * 2);
            uint16 *out = (uint16*)spec.vertices.data();
            for (const auto &it : lines.second)
            {
                for (uint32 pi = 0; pi < 2; pi++)
                    for (uint32 vi = 0; vi < 3; vi++)
                        *out++ = it.p[pi][vi];
            }
            spec.attributes[0].enable = true;
            spec.attributes[0].components = 3;
            spec.attributes[0].type = GpuTypeEnum::UnsignedShort;
            spec.faceMode = GpuMeshSpec::FaceMode::Lines;

            std::shared_ptr<GpuMesh> gm = std::make_shared<GpuMesh>(data->map,
                data->name + "#$!lines");
            data->map->callbacks.loadMesh(gm->info, spec);
            gm->state = Resource::State::ready;

            RenderTask task;
            task.mesh = gm;
            task.model = group->model();
            task.color = lines.first.color;
            data->renders.push_back(task);
        }

        cacheLines.clear();
    }

    // immutable data
    //   data given from the outside world

    GpuGeodata *const data;
    Value style;
    Value features;
    const uint32 lod;

    // processing data
    //   fast accessors to currently processed feature
    //   and the attached group, type and layer

    struct Group
    {
        const Value &group;

        Group(const Value &group)
            : group(group)
        {
            const Value &a = group["bbox"][0];
            vec3 aa = vec3(a[0].asDouble(), a[1].asDouble(), a[2].asDouble());
            const Value &b = group["bbox"][1];
            vec3 bb = vec3(b[0].asDouble(), b[1].asDouble(), b[2].asDouble());
            bboxOffset = aa;
            bboxScale = (bb - aa) / group["resolution"].asDouble();
        }

        vec3 point(sint32 a, sint32 b, sint32 c) const
        {
            return vec3(a, b, c).cwiseProduct(bboxScale) + bboxOffset;
        }

        vec3 point(uint32 a[3]) const
        {
            return point(a[0], a[1], a[2]);
        }

        vec3 point(const Value &v) const
        {
            return point(v[0].asUInt(), v[1].asUInt(), v[2].asUInt());
        }

        mat4 model() const
        {
            return translationMatrix(bboxOffset) * scaleMatrix(bboxScale);
        }

    private:
        vec3 bboxScale;
        vec3 bboxOffset;

    };
    boost::optional<Group> group;
    boost::optional<Type> type;

    struct Feature
    {
        const Value &feature;
        const Value &properties;

        Feature(const Value &feature)
            : feature(feature), properties(feature["properties"])
        {}
    };
    boost::optional<Feature> feature;

    // cache data
    //   temporary data generated while processing features
    //   these will be consumed in finishGroup
    //   which will generate the actual render tasks

    struct LineMutable
    {
        Point p[2];
    };
    struct LineImmutable
    {
        vec4f color;

        bool operator < (const LineImmutable &other) const
        {
            return memcmp(&color, &other.color, sizeof(color)) < 0;
            // todo consider all parameters
        }
    };
    std::map<LineImmutable, std::vector<LineMutable>> cacheLines;
};

} // namespace

void GpuGeodata::load()
{
    LOG(info2) << "Loading (gpu) geodata <" << name << ">";
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
