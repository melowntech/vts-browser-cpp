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

#include "../include/vts-browser/math.hpp"
#include "../utilities/json.hpp"
#include "../position.hpp"

#include <vts-libs/registry/json.hpp>
#include <vts-libs/registry/io.hpp>
#include <boost/lexical_cast.hpp>

namespace vts
{

Position::Position()
{
    point[0] = point[1] = point[2] = 0;
    orientation[0] = orientation[1] = orientation[2] = 0;
    viewExtent = 0;
    fov = 0;
    subjective = false;
    floatAltitude = false;
}

vtslibs::registry::Position p2p(const Position &p)
{
    vtslibs::registry::Position r;
    r.position = vecToUblas<math::Point3>(rawToVec3(p.point));
    r.orientation = vecToUblas<math::Point3>(rawToVec3(p.orientation));
    r.verticalExtent = p.viewExtent;
    r.verticalFov = p.fov;
    r.type = p.subjective ? vtslibs::registry::Position::Type::subjective : vtslibs::registry::Position::Type::objective;
    r.heightMode = p.floatAltitude ? vtslibs::registry::Position::HeightMode::floating : vtslibs::registry::Position::HeightMode::fixed;
    return r;
}

Position p2p(const vtslibs::registry::Position &p)
{
    Position r;
    vecToRaw(vecFromUblas<vec3>(p.position), r.point);
    vecToRaw(vecFromUblas<vec3>(p.orientation), r.orientation);
    r.viewExtent = p.verticalExtent;
    r.fov = p.verticalFov;
    r.subjective = p.type == vtslibs::registry::Position::Type::subjective;
    r.floatAltitude = p.heightMode == vtslibs::registry::Position::HeightMode::floating;
    return r;
}

Position::Position(const std::string &position)
{
    try
    {
        if (position.find("[") == std::string::npos)
        {
            *this = p2p(boost::lexical_cast<vtslibs::registry::Position>(position));
        }
        else
        {
            Json::Value val = stringToJson(position);
            *this = p2p(vtslibs::registry::positionFromJson(val));
        }
    }
    catch (const std::exception &e)
    {
        LOGTHROW(err2, std::runtime_error) << "Failed parsing position from string <" << position << "> with error <" << e.what() << ">";
    }
    catch (...)
    {
        LOGTHROW(err2, std::runtime_error) << "Failed parsing position from string <" << position << ">";
    }
}

std::string Position::toJson() const
{
    return jsonToString(vtslibs::registry::asJson(p2p(*this)));
}

std::string Position::toUrl() const
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(10);
    ss << p2p(*this);
    return ss.str();
}

} // namespace vts
