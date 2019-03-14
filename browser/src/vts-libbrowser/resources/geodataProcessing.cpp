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
#include "../include/vts-browser/log.hpp"

#include "../utilities/json.hpp"
#include "../utilities/case.hpp"
#include "../gpuResource.hpp"
#include "../geodata.hpp"
#include "../renderTasks.hpp"
#include "../map.hpp"

#include <utf8.h>

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
        int m = memcmp(a.model, b.model, sizeof(a.model));
        if (m != 0)
            return m < 0;
        return a.fontCascade < b.fontCascade;
    }
};

#define THROW LOGTHROW(err3, GeodataValidationException)

bool hasLatin(const std::string &s)
{
    auto it = s.begin();
    const auto e = s.end();
    while (it != e)
    {
        uint32 c = utf8::next(it, e);
        if ((c >= 0x41 && c <= 0x5a)
            || (c >= 0x61 && c <= 0x7a)
            || ((c >= 0xc0 && c <= 0xff) && c != 0xd7 && c != 0xf7)
            || (c >= 0x100 && c <= 0x17f))
            return true;
    }
    return false;
}

bool isCjk(const std::string &s)
{
    auto it = s.begin();
    const auto e = s.end();
    while (it != e)
    {
        uint32 c = utf8::next(it, e);
        if (!((c >= 0x4E00 && c <= 0x62FF) || (c >= 0x6300 && c <= 0x77FF) ||
            (c >= 0x7800 && c <= 0x8CFF) || (c >= 0x8D00 && c <= 0x9FFF) ||
            (c >= 0x3400 && c <= 0x4DBF) || (c >= 0x20000 && c <= 0x215FF) ||
            (c >= 0x21600 && c <= 0x230FF) || (c >= 0x23100 && c <= 0x245FF) ||
            (c >= 0x24600 && c <= 0x260FF) || (c >= 0x26100 && c <= 0x275FF) ||
            (c >= 0x27600 && c <= 0x290FF) || (c >= 0x29100 && c <= 0x2A6DF) ||
            (c >= 0x2A700 && c <= 0x2B73F) || (c >= 0x2B740 && c <= 0x2B81F) ||
            (c >= 0x2B820 && c <= 0x2CEAF) || (c >= 0x2CEB0 && c <= 0x2EBEF) ||
            (c >= 0xF900 && c <= 0xFAFF) || (c >= 0x3300 && c <= 0x33FF) ||
            (c >= 0xFE30 && c <= 0xFE4F) || (c >= 0xF900 && c <= 0xFAFF) ||
            (c >= 0x2F800 && c <= 0x2FA1F) ||
            (c <= 0x40) || (c >= 0xa0 && c <= 0xbf)))
            return false;
    }
    return true;
}

uint32 strlen(const std::string &s)
{
    uint32 result = 0;
    auto it = s.begin();
    const auto e = s.end();
    while (it != e)
    {
        utf8::next(it, e);
        result++;
    }
    return result;
}

double str2num(const std::string &s)
{
    if (s.empty())
        return nan1();
    std::stringstream ss(s);
    double f = nan1();
    ss >> std::noskipws >> f;
    if ((ss.rdstate() ^ std::ios_base::eofbit))
    {
        THROW << "Failed to convert string <"
            << s << "> to number";
    }
    return f;
}

bool isLayerStyleRequired(const Value &v)
{
    if (v.isNull())
        return false;
    if (v.isConvertibleTo(Json::ValueType::booleanValue))
        return v.asBool();
    return true;
}

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

    double convertToDouble(const Value &p) const
    {
        Value v = evaluate(p);
        if (v.isString())
            return str2num(v.asString());
        return v.asDouble();
    }

    Point convertPoint(const Value &p) const
    {
        return group->convertPoint(evaluate(p));
    }

    vec4f convertColor(const Value &p) const
    {
        Value v = evaluate(p);
        validateArrayLength(v, 4, 4, "Color must have 4 components");
        return vec4f(v[0].asInt(), v[1].asInt(), v[2].asInt(), v[3].asInt())
            / 255.f;
    }

    vec4f convertVector4(const Value &p) const
    {
        Value v = evaluate(p);
        validateArrayLength(v, 4, 4, "Expected 4 components");
        return vec4f(v[0].asFloat(), v[1].asFloat(),
            v[2].asFloat(), v[3].asFloat());
    }

    vec2f convertVector2(const Value &p) const
    {
        Value v = evaluate(p);
        validateArrayLength(v, 2, 2, "Expected 2 components");
        return vec2f(v[0].asFloat(), v[1].asFloat());
    }

    GpuGeodataSpec::Stick convertStick(const Value &p) const
    {
        Value v = evaluate(p);
        validateArrayLength(v, 7, 8, "Stick must have 7 or 8 components");
        GpuGeodataSpec::Stick s;
        s.heights[1] = v[0].asFloat();
        s.heights[0] = v[1].asFloat();
        s.width = v[2].asFloat();
        vecToRaw(vec4f(vec4f(v[3].asInt(), v[4].asInt(), v[5].asInt(),
            v[6].asInt()) / 255.f), s.color);
        s.offset = v.size() > 7 ? v[7].asFloat() : 0;
        return s;
    }

    GpuGeodataSpec::Origin convertOrigin(const Value &p) const
    {
        Value v = evaluate(p);
        std::string s = v.asString();
        if (s == "top-left")
            return GpuGeodataSpec::Origin::TopLeft;
        if (s == "top-right")
            return GpuGeodataSpec::Origin::TopRight;
        if (s == "top-center")
            return GpuGeodataSpec::Origin::TopCenter;
        if (s == "center-left")
            return GpuGeodataSpec::Origin::CenterLeft;
        if (s == "center-right")
            return GpuGeodataSpec::Origin::CenterRight;
        if (s == "center-center")
            return GpuGeodataSpec::Origin::CenterCenter;
        if (s == "bottom-left")
            return GpuGeodataSpec::Origin::BottomLeft;
        if (s == "bottom-right")
            return GpuGeodataSpec::Origin::BottomRight;
        if (s == "bottom-center")
            return GpuGeodataSpec::Origin::BottomCenter;
        if (Validating)
            THROW << "Invalid origin";
        return GpuGeodataSpec::Origin::Invalid;
    }

    GpuGeodataSpec::TextAlign convertTextAlign(const Value &p) const
    {
        Value v = evaluate(p);
        std::string s = v.asString();
        if (s == "left")
            return GpuGeodataSpec::TextAlign::Left;
        if (s == "right")
            return GpuGeodataSpec::TextAlign::Right;
        if (s == "center")
            return GpuGeodataSpec::TextAlign::Center;
        if (Validating)
            THROW << "Invalid text align";
        return GpuGeodataSpec::TextAlign::Invalid;
    }

    static void validateArrayLength(const Value &value,
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

    geoContext(GeodataTile *data,
        const GeodataStylesheet *style,
        const std::string &features,
        const vec3 aabbPhys[2], uint32 lod)
        : data(data),
        stylesheet(style),
        style(stringToJson(style->data)),
        features(stringToJson(features)),
        aabbPhys{ aabbPhys[0], aabbPhys[1] },
        lod(lod)
    {}

    // defines which style layers are candidates for a specific feature type
    std::vector<std::string> filterLayersByType(Type t) const
    {
        std::vector<std::string> result;
        const auto allLayerNames = style["layers"].getMemberNames();
        for (const std::string &layerName : allLayerNames)
        {
            Value layer = style["layers"][layerName];
            if (layer.isMember("filter") && layer["filter"].isArray()
                && layer["filter"][0] == "skip")
                continue;
            if (layer.isMember("visibility-switch"))
            {
                result.push_back(layerName);
                continue;
            }
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
                THROW << "Runtime error <" << e.what() << ">";
            }
            catch (const std::logic_error &e)
            {
                THROW << "Logic error <" << e.what() << ">";
            }
            catch (std::exception &e)
            {
                THROW << "General error <" << e.what() << ">";
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
                        processFeatureName(layerName);
                }
                this->feature.reset();
            }
            this->type.reset();
        }
        this->group.reset();

        // put cache into queue for upload
        data->specsToUpload.clear();
        for (const GpuGeodataSpec &spec : cacheData)
            data->specsToUpload.push_back(
                std::move(const_cast<GpuGeodataSpec&>(spec)));
    }

    Value resolveInheritance(const Value &orig) const
    {
        if (!orig["inherit"])
            return orig;

        Value base = resolveInheritance(
            style["layers"][orig["inherit"].asString()]);

        for (auto n : orig.getMemberNames())
            base[n] = orig[n];

        base.removeMember("inherit");

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
    Value replacement(const std::string &name) const
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

    Value replacementInternal(const std::string &name) const
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
            return (*feature)["properties"][name.substr(1)];
        case '#': // identifier
            if (name == "#id")
                return (*feature)["properties"]["name"];
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
    Value evaluate(const Value &expression) const
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

    Value evaluateInternal(const Value &expression) const
    {
        if (expression.isArray())
            return evaluateArray(expression);
        if (expression.isObject())
            return evaluateObject(expression);
        if (expression.isString())
            return evaluateString(expression.asString());
        return expression;
    }

    Value interpolate(const Value &a, const Value &b, double f) const
    {
        if (Validating)
        {
            if (a.isArray() != b.isArray())
                THROW << "Cannot interpolate <" << a.toStyledString()
                << "> and <" << b.toStyledString()
                << "> because one is array and the other is not";
            if (a.isArray() && a.size() != b.size())
            {
                THROW << "Cannot interpolate <" << a.toStyledString()
                    << "> and <" << b.toStyledString()
                    << "> because they have different number of elements";
            }
        }
        if (a.isArray())
        {
            Value r;
            Json::ArrayIndex cnt = a.size();
            for (Json::ArrayIndex i = 0; i < cnt; i++)
                r[i] = interpolate(a[i], b[i], f);
            return r;
        }
        double aa = convertToDouble(a);
        double bb = convertToDouble(b);
        return aa + (bb - aa) * f;
    }

    template<bool Linear>
    Value evaluatePairsArray(const Value &what,
        const Value &searchArray) const
    {
        if (Validating)
        {
            try
            {
                return evaluatePairsArrayInternal<Linear>(what, searchArray);
            }
            catch (...)
            {
                LOG(info3) << "In search of <"
                    << what.toStyledString()
                    << "> in pairs array <"
                    << searchArray.toStyledString() << ">";
                throw;
            }
        }
        return evaluatePairsArrayInternal<Linear>(what, searchArray);
    }

    template<bool Linear>
    Value evaluatePairsArrayInternal(const Value &what,
        const Value &searchArrayParam) const
    {
        const Value searchArray = evaluate(searchArrayParam);
        if (Validating)
        {
            if (!searchArray.isArray())
                THROW << "Expected an array";
            for (const auto &p : searchArray)
            {
                if (!p.isArray() || p.size() != 2)
                    THROW << "Expected an array with two elements";
            }
        }
        double v = convertToDouble(evaluate(what));
        for (sint32 index = searchArray.size() - 1; index >= 0; index--)
        {
            double v1 = convertToDouble(searchArray[index][0]);
            if (v < v1)
                continue;
            if (Linear)
            {
                if (index + 1u < searchArray.size())
                {
                    double v2 = convertToDouble(searchArray[index + 1][0]);
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

    Value evaluateArray(const Value &expression) const
    {
        Value r(expression);
        for (Value &it : r)
            it = evaluate(it);
        return r;
    }

    // evaluate function
    Value evaluateObject(const Value &expression) const
    {
        if (Validating)
        {
            if (expression.size() != 1)
                THROW << "Function must have exactly one member";
        }
        const std::string fnc = expression.getMemberNames()[0];

        // 'sgn', 'sin', 'cos', 'tan', 'asin', 'acos', 'atan', 'sqrt', 'abs', 'deg2rad', 'rad2deg', 'log'
        if (fnc == "sgn")
        {
            double v = convertToDouble(expression[fnc]);
            if (v < 0) return -1;
            if (v > 0) return 1;
            return 0;
        }
        if (fnc == "sin")
            return std::sin(convertToDouble(expression[fnc]));
        if (fnc == "cos")
            return std::cos(convertToDouble(expression[fnc]));
        if (fnc == "tan")
            return std::tan(convertToDouble(expression[fnc]));
        if (fnc == "asin")
            return std::asin(convertToDouble(expression[fnc]));
        if (fnc == "acos")
            return std::acos(convertToDouble(expression[fnc]));
        if (fnc == "atan")
            return std::atan(convertToDouble(expression[fnc]));
        if (fnc == "sqrt")
            return std::sqrt(convertToDouble(expression[fnc]));
        if (fnc == "abs")
            return std::abs(convertToDouble(expression[fnc]));
        if (fnc == "deg2rad")
            return convertToDouble(expression[fnc]) * M_PI / 180;
        if (fnc == "rad2deg")
            return convertToDouble(expression[fnc]) * 180 / M_PI;
        if (fnc == "log")
            return std::log(convertToDouble(expression[fnc]));

        // 'round'
        if (fnc == "round")
            return (sint32)std::round(convertToDouble(expression[fnc]));

        // 'add', 'sub', 'mul', 'div', 'pow', 'atan2'
#define COMP(NAME, OP) \
if (fnc == #NAME) \
{ \
    validateArrayLength(expression[#NAME], 2, 2, \
        "Function '" #NAME "' is expecting an array with 2 elements."); \
    double a = convertToDouble(expression[#NAME][0]); \
    double b = convertToDouble(expression[#NAME][1]); \
    return a OP b; \
}
        COMP(add, +);
        COMP(sub, -);
        COMP(mul, *);
        COMP(div, / );
#undef COMP
        if (fnc == "pow")
        {
            validateArrayLength(expression[fnc], 2, 2,
                "Function 'pow' is expecting an array with 2 elements.");
            double a = convertToDouble(expression[fnc][0]);
            double b = convertToDouble(expression[fnc][1]);
            return std::pow(a, b);
        }
        if (fnc == "atan2")
        {
            validateArrayLength(expression[fnc], 2, 2,
                "Function 'atan2' is expecting an array with 2 elements.");
            double a = convertToDouble(expression[fnc][0]);
            double b = convertToDouble(expression[fnc][1]);
            return std::atan2(a, b);
        }

        // 'clamp'
        if (fnc == "clamp")
        {
            const Value &arr = expression[fnc];
            validateArrayLength(arr, 3, 3,
                "Function 'clamp' must have 3 values");
            double f = convertToDouble(arr[0]);
            double a = convertToDouble(arr[1]);
            double b = convertToDouble(arr[2]);
            if (Validating)
            {
                if (a >= b)
                    THROW << "Function 'clamp' bound values <"
                    << a << "> and <" << b << "> are invalid";
            }
            return std::max(a, std::min(f, b));
        }

        // 'min', 'max'

        // 'if'
        if (fnc == "if")
        {
            const Value &arr = expression[fnc];
            validateArrayLength(arr, 3, 3,
                "Function 'if' must have 3 values");
            if (filter(arr[0]))
                return evaluate(arr[1]);
            else
                return evaluate(arr[2]);
        }

        // 'strlen', 'str2num', 'lowercase', 'uppercase', 'capitalize'
        if (fnc == "strlen")
            return strlen(evaluate(expression[fnc]).asString());
        if (fnc == "str2num")
            return str2num(evaluate(expression[fnc]).asString());
        if (fnc == "lowercase")
            return lowercase(evaluate(expression[fnc]).asString());
        if (fnc == "uppercase")
            return uppercase(evaluate(expression[fnc]).asString());
        if (fnc == "capitalize")
            return titlecase(evaluate(expression[fnc]).asString());

        // 'has-fonts', 'has-latin', 'is-cjk'
        if (fnc == "has-latin")
            return hasLatin(evaluate(expression[fnc]).asString());
        if (fnc == "is-cjk")
            return isCjk(evaluate(expression[fnc]).asString());

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
                "Function 'lod-scaled' must have 2 or 3 values");
            float l = arr[0].asFloat();
            float v = arr[1].asFloat();
            float bf = arr.size() == 3 ? arr[2].asFloat() : 1;
            return Value(std::pow(2 * bf, l - lod) * v);
        }

        // unknown
        if (Validating)
            THROW << "Unknown function <" << fnc << ">";
        return expression;
    }

    // string processing
    Value evaluateString(const std::string &s) const
    {
        if (Validating)
        {
            try
            {
                return evaluateStringInternal(s);
            }
            catch (...)
            {
                LOG(info3) << "In evaluation of string <"
                    << s << ">";
                throw;
            }
        }
        return evaluateStringInternal(s);
    }

    Value evaluateStringInternal(const std::string &s) const
    {
        // find '{'
        std::size_t start = s.find("{");
        if (start != s.npos)
        {
            size_t e = s.length();
            uint32 cnt = 1;
            std::size_t end = start;
            while (cnt > 0 && end + 1 < e)
            {
                end++;
                switch (s[end])
                {
                case '{': cnt++; break;
                case '}': cnt--; break;
                default: break;
                }
            }
            if (cnt > 0)
                THROW << "Missing '}' in <" << s << ">";
            if (end == start + 1)
                THROW << "Invalid '{}' in <" << s << ">";
            std::string subs = s.substr(start + 1, end - start - 1);
            Json::Value v;
            try
            {
                v = subs[0] == '{'
                    ? stringToJson(subs)
                    : Json::Value(subs);
            }
            catch (std::exception &e)
            {
                THROW << "Invalid json <" << subs
                    << ">, message <" << e.what() << ">";
            }
            subs = evaluate(v).asString();
            std::string res;
            if (start > 0)
                res += s.substr(0, start);
            res += subs;
            if (end < s.size())
                res += s.substr(end + 1);
            return evaluate(res);
        }
        // find '}'
        if (Validating)
        {
            auto end = s.find("}");
            if (end != s.npos)
                THROW << "Unmatched <}> in <" << s << ">";
        }
        // apply replacements
        return replacement(s);
    }

    // evaluation of expressions whose result is boolean
    bool filter(const Value &expression) const
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

    // some stylesheets contain an invalid aggregate filter
    //   in which the tests are all enclosed in additional array
    // this function tries to detect such cases and workaround it
    // example:
    //   invalid filter: ["all", [["has", "$foo"], ["has", "$bar"]]]
    //   correct filter: ["all", ["has", "$foo"], ["has", "$bar"]]
    const Value &aggregateFilterData(const Value &expression,
        uint32 &start) const
    {
        if (expression.size() == 2
            && expression[1].isArray()
            && expression[1][0].isArray())
        {
            start = 0;
            return expression[1];
        }
        else
        {
            start = 1;
            return expression;
        }
    }

    bool filterInternal(const Value &expression) const
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
                "Invalid filter 'skip' array length.");
            return false;
        }

        // comparison filters
        if (cond == "==" || cond == "!=")
        {
            validateArrayLength(expression, 3, 3, std::string()
                + "Invalid filter '" + cond + "' array length.");
            Value a = evaluate(expression[1]);
            Value b = evaluate(expression[2]);
            bool op = cond == "==";
            if ((a.isString() || a.isNull()) && (b.isString() || b.isNull()))
                return (a.asString() == b.asString()) == op;
            return (convertToDouble(a) == convertToDouble(b)) == op;
        }
#define COMP(OP) \
if (cond == #OP) \
{ \
    validateArrayLength(expression, 3, 3, \
            "Invalid filter '" #OP "' array length."); \
    double a = convertToDouble(expression[1]); \
    double b = convertToDouble(expression[2]); \
    return a OP b; \
}
        COMP(>= );
        COMP(<= );
        COMP(> );
        COMP(< );
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
                "Invalid filter 'has' array length.");
            Value a = expression[1];
            return !replacement(a.asString()).empty();
        }

        // in filters
        if (cond == "in")
        {
            validateArrayLength(expression, 2, -1,
                "Invalid filter 'in' array length.");
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
            uint32 start;
            const Value &v = aggregateFilterData(expression, start);
            for (uint32 i = start, e = v.size(); i < e; i++)
                if (!filter(v[i]))
                    return false;
            return true;
        }
        if (cond == "any")
        {
            uint32 start;
            const Value &v = aggregateFilterData(expression, start);
            for (uint32 i = start, e = v.size(); i < e; i++)
                if (filter(v[i]))
                    return true;
            return false;
        }
        if (cond == "none")
        {
            uint32 start;
            const Value &v = aggregateFilterData(expression, start);
            for (uint32 i = start, e = v.size(); i < e; i++)
                if (filter(v[i]))
                    return false;
            return true;
        }

        // unknown filter
        if (Validating)
            THROW << "Unknown filter condition type.";

        return false;
    }

    void addFont(const std::string &name,
        std::vector<std::shared_ptr<void>> &output) const
    {
        auto it = stylesheet->fonts.find(name);
        if (it == stylesheet->fonts.end())
            THROW << "Could not find font <" << name << ">";
        output.push_back(it->second->info.userData);
    }

    void findFonts(const Value &expression,
        std::vector<std::shared_ptr<void>> &output) const
    {
        if (!expression.empty())
        {
            Value v = evaluate(expression);
            validateArrayLength(v, 1, -1, "Fonts must be an array");
            for (const Value &nj : v)
            {
                std::string n = nj.asString();
                addFont(n, output);
            }
        }
        addFont("#default", output);
    }

    // process single feature with specific style layer
    void processFeatureName(const std::string &layerName)
    {
        std::array<float, 2> tv;
        tv[0] = -std::numeric_limits<float>::infinity();
        tv[1] = +std::numeric_limits<float>::infinity();
        const Value &layer = style["layers"][layerName];
        if (Validating)
        {
            try
            {
                return processFeature(layer, tv, {});
            }
            catch (...)
            {
                LOG(info3)
                    << "In feature <" << feature->toStyledString()
                    << "> and layer name <" << layerName << ">";
                throw;
            }
        }
        return processFeature(layer, tv, {});
    }

    void processFeature(const Value &layer,
        std::array<float, 2> tileVisibility,
        boost::optional<sint32> zOverride)
    {
        if (Validating)
        {
            try
            {
                return processFeatureInternal(layer,
                    tileVisibility, zOverride);
            }
            catch (...)
            {
                LOG(info3) << "In layer <" << layer.toStyledString() << ">";
                throw;
            }
        }
        return processFeatureInternal(layer, tileVisibility, zOverride);
    }

    void processFeatureInternal(const Value &layer,
        std::array<float, 2> tileVisibility,
        boost::optional<sint32> zOverride)
    {
        // filter
        if (!zOverride && layer.isMember("filter")
            && !filter(layer["filter"]))
            return;

        // visible
        if (layer.isMember("visible")
            && !evaluate(layer["visible"]).asBool())
            return;

        // next-pass
        {
            auto np = layer["next-pass"];
            if (!np.empty())
            {
                std::string layerName = np[1].asString();
                if (Validating)
                {
                    if (style["layers"][layerName].empty())
                        THROW << "Invalid layer name <"
                        << layerName << "> in next-pass";
                }
                processFeature(style["layers"][layerName],
                    tileVisibility, np[0].asInt());
            }
        }

        // visibility-switch
        if (layer.isMember("visibility-switch"))
        {
            validateArrayLength(layer["visibility-switch"], 1, -1,
                "Visibility-switch must be an array.");
            float ve = -std::numeric_limits<float>::infinity();
            for (const Value &vs : layer["visibility-switch"])
            {
                validateArrayLength(vs, 2, 2, "All visibility-switch "
                    "elements must be arrays with 2 elements");
                float a = evaluate(vs[0]).asFloat();
                if (Validating)
                {
                    if (a <= ve)
                        THROW << "Values in visibility-switch "
                        "must be increasing";
                }
                if (!vs[1].empty())
                {
                    std::array<float, 2> tv;
                    tv[0] = std::max(tileVisibility[0], ve);
                    tv[1] = std::min(tileVisibility[1], a);
                    if (tv[0] < tv[1])
                    {
                        std::string ln = evaluate(vs[1]).asString();
                        Value l = layer;
                        l.removeMember("filter");
                        l.removeMember("visible");
                        l.removeMember("next-pass");
                        l.removeMember("visibility-switch");
                        const Value &s = style["layers"][ln];
                        for (auto n : s.getMemberNames())
                            l[n] = s[n];
                        l["filter"] = layer["filter"];
                        processFeature(l, tv, zOverride);
                    }
                }
                ve = a;
            }
            return;
        }

        GpuGeodataSpec spec;
        spec.commonData.tileVisibility[0] = tileVisibility[0];
        spec.commonData.tileVisibility[1] = tileVisibility[1];
        processFeatureCommon(layer, spec, zOverride);

        // line
        if (evaluate(layer["line"]).asBool())
            processFeatureLine(layer, spec);

        // line-label
        if (evaluate(layer["line-label"]).asBool())
            processFeatureLineLabel(layer, spec);

        // point
        if (evaluate(layer["point"]).asBool())
            processFeaturePoint(layer, spec);

        // icon

        // point label
        if (evaluate(layer["label"]).asBool())
            processFeaturePointLabel(layer, spec);

        // polygon
    }

    void processFeatureCommon(const Value &layer, GpuGeodataSpec &spec,
        boost::optional<sint32> zOverride) const
    {
        // model matrix
        matToRaw(group->model, spec.model);

        // z-index
        if (zOverride)
            spec.commonData.zIndex = *zOverride;
        else if (layer.isMember("z-index"))
            spec.commonData.zIndex = evaluate(layer["z-index"]).asInt();

        // zbuffer-offset
        if (layer.isMember("zbuffer-offset"))
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
        if (layer.isMember("visibility"))
        {
            spec.commonData.visibilities[0]
                = evaluate(layer["visibility"]).asFloat();
        }

        // visibility-abs
        if (layer.isMember("visibility-abs"))
        {
            Value arr = evaluate(layer["visibility-abs"]);
            validateArrayLength(arr, 2, 2,
                "visibility-abs must have 2 values");
            for (int i = 0; i < 2; i++)
                spec.commonData.visibilities[i + 1] = arr[i].asFloat();
        }

        // visibility-rel
        if (layer.isMember("visibility-rel"))
        {
            Value arr = evaluate(layer["visibility-rel"]);
            validateArrayLength(arr, 4, 4,
                "visibility-rel must have 4 values");
            float d = arr[0].asFloat() * arr[1].asFloat();
            float v3 = arr[3].asFloat();
            float v2 = arr[2].asFloat();
            vec2f vr = vec2f(v3 <= 0 ? 0 : d / v3, v2 <= 0 ? 0 : d / v2);
            float *vs = spec.commonData.visibilities;
            if (vs[1] == vs[1])
            {
                // merge with visibility-abs
                vs[1] = std::max(vs[1], vr[0]);
                vs[2] = std::min(vs[2], vr[1]);
            }
            else
            {
                vs[1] = vr[0];
                vs[2] = vr[1];
            }
        }

        // culling
        if (layer.isMember("culling"))
            spec.commonData.visibilities[3]
            = evaluate(layer["culling"]).asFloat();
    }

    void processFeatureLine(const Value &layer, GpuGeodataSpec spec)
    {
        if (evaluate(layer["line-flat"]).asBool())
            spec.type = GpuGeodataSpec::Type::LineFlat;
        else
            spec.type = GpuGeodataSpec::Type::LineScreen;
        vecToRaw(convertColor(layer["line-color"]),
            spec.unionData.line.color);
        spec.unionData.line.width
            = evaluate(layer["line-width"]).asFloat();
        if (layer.isMember("line-width-units"))
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
                if (spec.type == GpuGeodataSpec::Type::LineFlat
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
        else if (spec.type == GpuGeodataSpec::Type::LineFlat)
            spec.unionData.line.units = GpuGeodataSpec::Units::Meters;
        else
            spec.unionData.line.units = GpuGeodataSpec::Units::Pixels;
        GpuGeodataSpec &data = findSpecData(spec);
        const auto arr = getFeaturePositions();
        data.positions.reserve(data.positions.size() + arr.size());
        data.positions.insert(data.positions.end(), arr.begin(), arr.end());
    }

    void processFeaturePoint(const Value &layer, GpuGeodataSpec spec)
    {
        if (evaluate(layer["point-flat"]).asBool())
            spec.type = GpuGeodataSpec::Type::PointFlat;
        else
            spec.type = GpuGeodataSpec::Type::PointScreen;
        vecToRaw(layer.isMember("point-color")
            ? convertColor(layer["point-color"])
            : vec4f(1, 1, 1, 1),
            spec.unionData.point.color);
        spec.unionData.point.radius
            = layer.isMember("point-radius")
            ? evaluate(layer["point-radius"]).asFloat()
            : 1;
        GpuGeodataSpec &data = findSpecData(spec);
        auto arr = getFeaturePositions();
        cullOutsideFeatures(arr);
        data.positions.reserve(data.positions.size() + arr.size());
        data.positions.insert(data.positions.end(), arr.begin(), arr.end());
    }

    void processFeatureLineLabel(const Value &layer, GpuGeodataSpec spec)
    {
        (void)layer;
        (void)spec;
        // todo
    }

    void processFeaturePointLabel(const Value &layer, GpuGeodataSpec spec)
    {
        findFonts(layer["label-font"], spec.fontCascade);
        spec.type = GpuGeodataSpec::Type::PointLabel;
        vecToRaw(layer.isMember("label-color")
            ? convertColor(layer["label-color"])
            : vec4f(1, 1, 1, 1),
            spec.unionData.pointLabel.color);
        vecToRaw(layer.isMember("label-color2")
            ? convertColor(layer["label-color2"])
            : vec4f(0, 0, 0, 1),
            spec.unionData.pointLabel.color2);
        vecToRaw(layer.isMember("label-outline")
            ? convertVector4(layer["label-outline"])
            : vec4f(0.25, 0.75, 2.2, 2.2),
            spec.unionData.pointLabel.outline);
        vecToRaw(layer.isMember("label-offset")
            ? convertVector2(layer["label-offset"])
            : vec2f(0, 0),
            spec.unionData.pointLabel.offset);
        if (!layer.isMember("label-no-overlap")
            || evaluate(layer["label-no-overlap"]).asBool())
        {
            const Json::Value &lnom = layer["label-no-overlap-margin"];
            vecToRaw(lnom.empty()
                ? vec4f(5, 5, 0, 0)
                : lnom.size() == 4
                ? convertVector4(lnom)
                : vec3to4(vec2to3(convertVector2(lnom), 0), 0),
                spec.unionData.pointLabel.margin);
        }
        spec.unionData.pointLabel.size
            = layer.isMember("label-size")
            ? evaluate(layer["label-size"]).asFloat()
            : 10;
        spec.unionData.pointLabel.width
            = layer.isMember("label-width")
            ? evaluate(layer["label-width"]).asFloat()
            : 200;
        spec.unionData.pointLabel.origin
            = layer.isMember("label-origin")
            ? convertOrigin(layer["label-origin"])
            : GpuGeodataSpec::Origin::BottomCenter;
        spec.unionData.pointLabel.textAlign
            = layer.isMember("label-align")
            ? convertTextAlign(layer["label-align"])
            : GpuGeodataSpec::TextAlign::Center;
        if (layer.isMember("label-stick"))
            spec.commonData.stick
            = convertStick(layer["label-stick"]);
        GpuGeodataSpec &data = findSpecData(spec);
        auto arr = getFeaturePositions();
        cullOutsideFeatures(arr);
        data.positions.reserve(data.positions.size() + arr.size());
        data.positions.insert(data.positions.end(), arr.begin(), arr.end());
        std::string text = evaluate(layer.isMember("label-source")
            ? layer["label-source"] : "$name").asString();
        data.texts.reserve(data.texts.size() + arr.size());
        for (const auto &a : arr)
        {
            (void)a; // is this suspicious?
            data.texts.push_back(text);
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
    const GeodataStylesheet *const stylesheet;
    Value style;
    Value features;
    const vec3 aabbPhys[2];
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
            validateArrayLength(v, 3, 3, "Point must have 3 coordinates");
            vec3f p(v[0].asFloat(), v[1].asFloat(), v[2].asFloat());
            p = orthonormalize * p;
            return Point({ p[0], p[1], p[2] });
        }

        std::vector<Point> convertArray(const Value &v, bool relative) const
        {
            (void)relative; // todo
            std::vector<Point> a;
            a.reserve(v.size());
            for (const Value &p : v)
                a.push_back(convertPoint(p));
            return a;
        }

        mat4 model;

    private:
        mat3f orthonormalize;
    };

    boost::optional<Group> group;
    boost::optional<Type> type;
    boost::optional<const Value &> feature;

    std::vector<std::vector<Point>> getFeaturePositions() const
    {
        std::vector<std::vector<Point>> result;
        switch (*type)
        {
        case Type::Point:
        {
            const Value &array1 = (*feature)["points"];
            result.push_back(
                group->convertArray(array1, false));
            // todo d-points
        } break;
        case Type::Line:
        {
            const Value &array1 = (*feature)["lines"];
            for (const Value &array2 : array1)
            {
                result.push_back(
                    group->convertArray(array2, false));
            }
            // todo d-lines
        } break;
        case Type::Polygon:
        {
            // todo
        } break;
        }
        return result;
    }

    void cullOutsideFeatures(std::vector<std::vector<Point>> &fps) const
    {
        for (auto &v : fps)
        {
            v.erase(std::remove_if(v.begin(), v.end(),
                [&](const Point &p) {
                vec3 a = vec4to3(vec4(group->model
                    * vec3to4(rawToVec3(p.data()).cast<double>(), 1.0)));
                for (int i = 0; i < 3; i++)
                {
                    if (a[i] < aabbPhys[0][i])
                        return true;
                    if (a[i] > aabbPhys[1][i])
                        return true;
                }
                return false;
            }), v.end());
        }
        fps.erase(std::remove_if(fps.begin(), fps.end(),
            [&](const std::vector<Point> &v) {
            return v.empty();
        }), fps.end());
    }

    // cache data
    //   temporary data generated while processing features

    std::set<GpuGeodataSpec, GpuGeodataSpecComparator> cacheData;
};

} // namespace

void GeodataTile::load()
{
    LOG(info2) << "Loading (gpu) geodata <" << name << ">";

    // this resource is not meant to be downloaded
    assert(!fetch);

    // upload
    renders.clear();
    renders.reserve(specsToUpload.size());
    uint32 index = 0;
    for (auto &spec : specsToUpload)
    {
        RenderGeodataTask t;
        t.geodata = std::make_shared<GpuGeodata>();
        std::stringstream ss;
        ss << name << "#$!" << index++;
        map->callbacks.loadGeodata(t.geodata->info, spec, ss.str());
        renders.push_back(t);
    }
    std::vector<GpuGeodataSpec>().swap(specsToUpload);

    // memory consumption
    info.ramMemoryCost = sizeof(*this)
        + renders.size() * sizeof(RenderGeodataTask);
    for (const RenderGeodataTask &it : renders)
    {
        info.gpuMemoryCost += it.geodata->info.gpuMemoryCost;
        info.ramMemoryCost += it.geodata->info.ramMemoryCost;
    }
}

void GeodataTile::process()
{
    LOG(info2) << "Processing geodata <" << name << ">";

    if (map->options.debugValidateGeodataStyles)
    {
        geoContext<true> ctx(this, style.get(), *features, aabbPhys, lod);
        ctx.process();
    }
    else
    {
        geoContext<false> ctx(this, style.get(), *features, aabbPhys, lod);
        ctx.process();
    }

    state = Resource::State::downloaded;
    map->resources.queUpload.push(shared_from_this());
}

void MapImpl::resourcesGeodataProcessorEntry()
    {
        setLogThreadName("geodata processor");
        while (!resources.queGeodata.stopped())
        {
            std::weak_ptr<GeodataTile> w;
            resources.queGeodata.waitPop(w);
            std::shared_ptr<GeodataTile> r = w.lock();
            if (!r)
                continue;
            try
            {
                r->process();
            }
            catch (const std::exception &)
            {
                r->state = Resource::State::errorFatal;
            }
        }
    }

} // namespace vts
