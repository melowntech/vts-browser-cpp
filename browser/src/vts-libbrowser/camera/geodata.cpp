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

#include "../include/vts-browser/exceptions.hpp"

#include "../utilities/json.hpp"
#include "../camera.hpp"
#include "../gpuResource.hpp"
#include "../geodata.hpp"
#include "../renderTasks.hpp"
#include "../map.hpp"

namespace vts
{

using Json::Value;

namespace
{

struct GpuGeodataSpecComparator
{
    bool operator () (const GpuGeodataSpec &a, const GpuGeodataSpec &b) const
    {
        if (a.type != b.type)
            return a.type < b.type;
        int c = memcmp(&a.commonData, &b.commonData, sizeof(a.commonData));
        if (c != 0)
            return c < 0;
        int s = memcmp(&a.unionData, &b.unionData, sizeof(a.unionData));
        if (s != 0)
            return s < 0;
        return memcmp(a.model, b.model, sizeof(float) * 16) < 0;
    }
};

#define THROW LOGTHROW(err3, GeodataValidationException)

template<bool Validating>
struct geoContext
{
    enum class Type
    {
        Point,
        Line,
        Polygon,
    };

    typedef std::array<float, 3> Point;

    Point convertPoint(const Value &v)
    {
        if (Validating)
            validateArrayLength(v, 3, 3, "Point must have 3 coordinates");
        return group->convertPoint(v);
    }

    vec4f convertColor(const Value &v)
    {
        return vec4f(v[0].asInt(), v[1].asInt(), v[2].asInt(), v[3].asInt())
                / 255.f;
    }

    void validateArrayLength(const Value &value,
                             uint32 minimum, uint32 maximum, // inclusive range
                             const std::string &message)
    {
        if (Validating)
        {
            if (!value.isArray() || value.size() < minimum
                    || value.size() > maximum)
                THROW << message;
        }
    }

    geoContext(GeodataTile *data, const std::string &style,
               const std::string &features, uint32 lod)
        : data(data),
          style(stringToJson(style)),
          features(stringToJson(features)),
          lod(lod)
    {}

    bool isLayerStyleRequired(const Value &v)
    {
        if (v.isNull())
            return false;
        if (v.isConvertibleTo(Json::ValueType::booleanValue))
            return v.asBool();
        return true;
    }

    // defines which style layers are candidates for a specific feature type
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

    // entry point
    //   processes all features with all style layers
    void process()
    {
        if (Validating)
        {
            try
            {
                return processInternal();
            }
            catch (const GeodataValidationException &)
            {
                throw;
            }
            catch (const std::runtime_error &e)
            {
                throw GeodataValidationException(e.what());
            }
        }
        return processInternal();
    }

    void processInternal()
    {
        if (Validating)
        {
            // check version
            if (features["version"].asInt() != 1)
            {
                THROW << "Invalid geodata features <"
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
        }

        // convert cache into actual render tasks
        for (const GpuGeodataSpec &spec : cacheData)
        {
            RenderGeodataTask t;
            t.geodata = std::make_shared<GpuGeodata>();
            data->map->callbacks.loadGeodata(t.geodata->info,
                                const_cast<GpuGeodataSpec&>(spec));
            data->renders.push_back(t);
        }
    }

    Value resolveInheritance(const Value &orig)
    {
        if (!orig["inheritance"])
            return orig;

        Value base = resolveInheritance(
            style["layers"][orig["inheritance"].asString()]);

        for (auto n : orig.getMemberNames())
            base[n] = orig[n];

        return base;
    }

    // update style layers with inherit property
    void solveInheritance()
    {
        Value ls;
        for (const std::string &n : style["layers"].getMemberNames())
            ls[n] = resolveInheritance(style["layers"][n]);
        style["layers"] = ls;
    }

    // solves @constants, $properties and #identifiers
    Value replacement(const std::string &name)
    {
        if (Validating)
        {
            try
            {
                return replacementInternal(name);
            }
            catch (...)
            {
                LOG(info3) << "In replacements of <" << name << ">";
                throw;
            }
        }
        return replacementInternal(name);
    }

    Value replacementInternal(const std::string &name)
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
            if (name == "#lod")
                return lod;
            if (name == "#tileSize")
            {
                // todo
                LOGTHROW(fatal, std::logic_error)
                        << "#tileSize is not yet implemented";
            }
            return Value();
        default:
            return name;
        }
    }

    // solves functions and string expansion
    Value evaluate(const Value &expression)
    {
        if (Validating)
        {
            try
            {
                return evaluateInternal(expression);
            }
            catch (...)
            {
                LOG(info3) << "In evaluation of <"
                    << expression.toStyledString() << ">";
                throw;
            }
        }
        return evaluateInternal(expression);
    }

    Value interpolate(const Value &a, const Value &b, double f)
    {
        LOGTHROW(fatal, std::logic_error)
            << "interpolate is not yet implemented";
        throw;
        (void)a;
        (void)b;
        (void)f;
    }

    template<bool Linear>
    Value evaluatePairsArray(const Value &what, const Value &searchArray)
    {
        if (Validating)
        {
            if (!searchArray.isArray())
                THROW << "Expected an array";
            for (const auto p : searchArray)
            {
                if (!p.isArray() || p.size() != 2)
                    THROW << "Expected an array with two elements";
            }
        }
        double v = evaluate(what).asDouble();
        for (sint32 index = searchArray.size() - 1; index >= 0; index--)
        {
            double v1 = evaluate(searchArray[index][0]).asDouble();
            if (v < v1)
                continue;
            if (Linear)
            {
                if (index + 1u < searchArray.size())
                {
                    double v2 = evaluate(searchArray[index + 1][0]).asDouble();
                    return interpolate(
                        evaluate(searchArray[index + 0][1]),
                        evaluate(searchArray[index + 1][1]),
                        (v - v1) / (v2 - v1));
                }
            }
            return evaluate(searchArray[index][1]);
        }
        return evaluate(searchArray[0][1]);
    }

    Value evaluateInternal(const Value &expression)
    {
        if (expression.isArray())
        {
            Value r(expression);
            for (Value &it : r)
                it = evaluate(it);
            return r;
        }

        if (expression.isObject())
        {
            if (Validating)
            {
                if (expression.size() != 1)
                    THROW << "Function must have exactly one member";
            }
            const std::string fnc = expression.getMemberNames()[0];

            // 'sgn', 'sin', 'cos', 'tan', 'asin', 'acos', 'atan', 'sqrt', 'abs', 'deg2rad', 'rad2deg'

            // 'add', 'sub', 'mul', 'div', 'pow', 'atan2'

            // 'min', 'max'

            // 'if'

            // 'strlen', 'str2num', 'lowercase', 'uppercase', 'capitalize'

            // 'has-fonts', 'has-latin', 'is-cjk'

            // 'discrete', 'discrete2', 'linear', 'linear2'
            if (fnc == "discrete")
                return evaluatePairsArray<false>(lod, expression[fnc]);
            if (fnc == "discrete2")
                return evaluatePairsArray<false>(expression[fnc][0],
                                                 expression[fnc][1]);
            if (fnc == "linear")
                return evaluatePairsArray<true>(lod, expression[fnc]);
            if (fnc == "linear2")
                return evaluatePairsArray<true>(expression[fnc][0],
                                                expression[fnc][1]);

            // 'lod-scaled'
            if (fnc == "lod-scaled")
            {
                Value arr = evaluate(expression["lod-scaled"]);
                validateArrayLength(arr, 2, 3,
                    "lod-scaled must have 2 or 3 values");
                float l = arr[0].asFloat();
                float v = arr[1].asFloat();
                float bf = arr.size() == 3 ? arr[2].asFloat() : 1;
                return Value(std::pow(2 * bf, l - lod) * v);
            }

            // unknown
            if (Validating)
                THROW << "Unknown function <" << fnc << ">";
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
                            THROW << "Unmatched <{> in <"
                                  << expression.toStyledString() << ">";
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
                        + evaluate(stringToJson(mid)).asString()
                        + s.substr(end + 1);
                return evaluate(res);
            }
            // find '}'
            if (Validating)
            {
                auto end = s.find("}");
                if (end != s.npos)
                    THROW << "Unmatched <}> in <"
                          << expression.toStyledString() << ">";
            }
            // apply replacements
            return replacement(s);
        }
        return expression;
    }

    // evaluation of expressions whose result is boolean
    bool filter(const Value &expression)
    {
        if (Validating)
        {
            try
            {
                return filterInternal(expression);
            }
            catch (...)
            {
                LOG(info3) << "In filter <"
                    << expression.toStyledString() << ">";
                throw;
            }
        }
        return filterInternal(expression);
    }

    bool filterInternal(const Value &expression)
    {
        if (Validating)
        {
            if (!expression.isArray())
                THROW << "Filter must be array.";
        }

        std::string cond = expression[0].asString();

        if (cond == "skip")
        {
            validateArrayLength(expression, 1, 1,
                    "Invalid filter (skip) array length.");
            return false;
        }

        // comparison filters
#define COMP(OP) \
        if (cond == #OP) \
        { \
            validateArrayLength(expression, 3, 3, \
                    "Invalid filter (" #OP ") array length."); \
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
            validateArrayLength(expression, 2, -1,
                    "Invalid filter (has) array length.");
            Value a = expression[1];
            return a != replacement(a.asString());
        }

        // in filters
        if (cond == "in")
        {
            validateArrayLength(expression, 2, -1,
                    "Invalid filter (in) array length.");
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
            THROW << "Unknown filter condition type.";

        return false;
    }

    // process single feature with specific style layer
    void processFeature(const std::string &layerName,
        boost::optional<sint32> zOverride
        = boost::optional<sint32>())
    {
        if (Validating)
        {
            try
            {
                return processFeatureInternal(layerName, zOverride);
            }
            catch (...)
            {
                LOG(info3) << "In layer <" << layerName << ">";
                throw;
            }
        }
        return processFeatureInternal(layerName, zOverride);
    }

    void processFeatureInternal(const std::string &layerName,
                        boost::optional<sint32> zOverride
                                = boost::optional<sint32>())
    {
        const Value &layer = style["layers"][layerName];

        // filter
        if (!zOverride && !filter(layer["filter"]))
            return;

        // visible
        if (layer["visible"])
            if (!evaluate(layer["visible"]).asBool())
                return;

        GpuGeodataSpec spec;
        processFeatureCommon(layer, spec, zOverride);

        // line
        if (evaluate(layer["line"]).asBool())
            processFeatureLine(layer, spec);

        // line-label

        // point
        if (evaluate(layer["point"]).asBool())
            processFeaturePoint(layer, spec);

        // icon

        // label

        // polygon

        // next-pass
        {
            auto np = layer["next-pass"];
            if (np)
                processFeature(np[1].asString(), np[0].asInt());
        }
    }

    void processFeatureCommon(const Value &layer, GpuGeodataSpec &spec,
        boost::optional<sint32> zOverride)
    {
        // model matrix
        matToRaw(group->model, spec.model);

        // z-index
        if (zOverride)
            spec.commonData.zIndex = *zOverride;
        else if (layer["z-index"])
            spec.commonData.zIndex = evaluate(layer["z-index"]).asInt();

        // zbuffer-offset
        if (layer["zbuffer-offset"])
        {
            Value arr = evaluate(layer["zbuffer-offset"]);
            validateArrayLength(arr, 3, 3,
                "zbuffer-offset must have 3 values");
            for (int i = 0; i < 3; i++)
                spec.commonData.zBufferOffset[i] = arr[i].asFloat();
        }
        else
        {
            for (int i = 0; i < 3; i++)
                spec.commonData.zBufferOffset[i] = 0;
        }

        // visibility
        if (layer["visibility"])
            spec.commonData.visibility
                = evaluate(layer["visibility"]).asFloat();

        // visibility-abs
        if (layer["visibility-abs"])
        {
            Value arr = evaluate(layer["visibility-abs"]);
            validateArrayLength(arr, 2, 2,
                "visibility-abs must have 2 values");
            for (int i = 0; i < 2; i++)
                spec.commonData.visibilityAbsolute[i] = arr[i].asFloat();
        }

        // visibility-rel
        if (layer["visibility-rel"])
        {
            Value arr = evaluate(layer["visibility-rel"]);
            validateArrayLength(arr, 4, 4,
                "visibility-rel must have 4 values");
            for (int i = 0; i < 4; i++)
                spec.commonData.visibilityRelative[i] = arr[i].asFloat();
        }

        // culling
        if (layer["culling"])
            spec.commonData.culling = evaluate(layer["culling"]).asFloat();
    }

    void processFeatureLine(const Value &layer, GpuGeodataSpec spec)
    {
        if (evaluate(layer["line-flat"]).asBool())
            spec.type = GpuGeodataSpec::Type::LineWorld;
        else
            spec.type = GpuGeodataSpec::Type::LineScreen;
        vecToRaw(convertColor(layer["line-color"]),
            spec.unionData.line.color);
        spec.unionData.line.width
            = evaluate(layer["line-width"]).asFloat();
        if (layer["line-width-units"])
        {
            std::string units = evaluate(layer["line-width-units"]).asString();
            if (units == "ratio")
                spec.unionData.line.units = GpuGeodataSpec::Units::Ratio;
            else if (units == "pixels")
                spec.unionData.line.units = GpuGeodataSpec::Units::Pixels;
            else if (units == "meters")
                spec.unionData.line.units = GpuGeodataSpec::Units::Meters;
            else if (Validating)
                THROW << "Invalid line-width-units";
            if (Validating)
            {
                if (spec.type == GpuGeodataSpec::Type::LineWorld
                    && spec.unionData.line.units
                    == GpuGeodataSpec::Units::Pixels)
                    THROW
                        << "Invalid combination of line-width-units"
                        << " (pixels) and line-flat (true)";
                if (spec.type == GpuGeodataSpec::Type::LineScreen
                    && spec.unionData.line.units
                    == GpuGeodataSpec::Units::Meters)
                    THROW
                        << "Invalid combination of line-width-units"
                        << " (meters) and line-flat (false)";
            }
        }
        else if (spec.type == GpuGeodataSpec::Type::LineWorld)
            spec.unionData.line.units = GpuGeodataSpec::Units::Meters;
        else
            spec.unionData.line.units = GpuGeodataSpec::Units::Pixels;
        GpuGeodataSpec &data = findSpecData(spec);
        switch (*type)
        {
        case Type::Point:
            // nothing
            break;
        case Type::Line:
        {
            const Value &array1 = feature->feature["lines"];
            for (const Value &array2 : array1)
            {
                if (array2.empty())
                    continue;
                std::vector<Point> v; // separate lines
                for (const Value &point : array2)
                    v.push_back(convertPoint(point));
                data.coordinates.push_back(v);
            }
        } break;
        case Type::Polygon:
        {
            // todo
        } break;
        }
    }

    void processFeaturePoint(const Value &layer, GpuGeodataSpec spec)
    {
        if (evaluate(layer["point-flat"]).asBool())
            spec.type = GpuGeodataSpec::Type::PointWorld;
        else
            spec.type = GpuGeodataSpec::Type::PointScreen;
        vecToRaw(convertColor(layer["point-color"]),
            spec.unionData.point.color);
        spec.unionData.point.radius
            = evaluate(layer["point-radius"]).asFloat();
        GpuGeodataSpec &data = findSpecData(spec);
        switch (*type)
        {
        case Type::Point:
        case Type::Line:
        {
            const Value &array1 = feature->feature[
                *type == Type::Point ? "points"
                    : *type == Type::Line ? "lines"
                    : ""];
            std::vector<Point> v; // merge all points into single array
            for (const Value &array2 : array1)
            {
                if (array2.empty())
                    continue;
                for (const Value &point : array2)
                    v.push_back(convertPoint(point));
            }
            data.coordinates.push_back(v);
        } break;
        case Type::Polygon:
        {
            // todo
        } break;
        }
    }

    GpuGeodataSpec &findSpecData(const GpuGeodataSpec &spec)
    {
        // only modifying attributes not used in comparison
        auto specIt = cacheData.find(spec);
        if (specIt == cacheData.end())
            specIt = cacheData.insert(spec).first;
        GpuGeodataSpec &data = const_cast<GpuGeodataSpec&>(*specIt);
        return data;
    }

    // immutable data
    //   data given from the outside world

    GeodataTile *const data;
    Value style;
    Value features;
    const uint32 lod;

    // processing data
    //   fast accessors to currently processed feature
    //   and the attached group, type and layer

    struct Group
    {
        const Value &group;

        Group(const Value &group) : group(group)
        {
            const Value &a = group["bbox"][0];
            vec3 aa = vec3(a[0].asDouble(), a[1].asDouble(), a[2].asDouble());
            const Value &b = group["bbox"][1];
            vec3 bb = vec3(b[0].asDouble(), b[1].asDouble(), b[2].asDouble());
            double resolution = group["resolution"].asDouble();
            vec3 mm = bb - aa;
            //double am = std::max(mm[0], std::max(mm[1], mm[2]));
            orthonormalize = mat4to3(scaleMatrix(
                        mm / resolution)).cast<float>();
            model = translationMatrix(aa) * scaleMatrix(1);
        }

        Point convertPoint(const Value &v) const
        {
            vec3f p(v[0].asUInt(), v[1].asUInt(), v[2].asUInt());
            p = orthonormalize * p;
            return Point({ p[0], p[1], p[2] });
        }

        mat4 model;

    private:
        mat3f orthonormalize;
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

    std::set<GpuGeodataSpec, GpuGeodataSpecComparator> cacheData;
};

} // namespace

void GeodataTile::load()
{
    LOG(info2) << "Loading (gpu) geodata <" << name << ">";
    renders.clear();

    // this resource is not meant to be downloaded
    assert(!fetch);

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

    // memory consumption
    info.ramMemoryCost += sizeof(*this)
        + renders.size() * sizeof(RenderGeodataTask);
    for (const RenderGeodataTask &it : renders)
    {
        info.gpuMemoryCost += it.geodata->info.gpuMemoryCost;
        info.ramMemoryCost += it.geodata->info.ramMemoryCost;
    }
}

} // namespace vts