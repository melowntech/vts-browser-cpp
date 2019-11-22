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

#include <vts-browser/log.hpp>
#include <vts-browser/math.hpp>
#include <vts-browser/position.hpp>

#include <vector>
#include <sstream>
#include <iomanip>

std::string jsonToHtml(const std::string &json)
{
    std::stringstream ss;
    std::vector<char> type;
    type.push_back('s');
    for (char c : json)
    {
        switch (c)
        {
        case '{':
            type.push_back('s');
            ss << "<table><tr><td>";
            break;
        case '}':
            type.pop_back();
            ss << "</tr></table>";
            break;
        case '[':
            type.push_back('a');
            ss << "<div>";
            break;
        case ']':
            type.pop_back();
            ss << "</div>";
            break;
        case ':':
            ss << "<td class=number>";
            break;
        case ',':
        {
            switch (type.back())
            {
            case 's':
                ss << "<tr><td>";
                break;
            case 'a':
                ss << "</div><div>";
                break;
            }
        } break;
        case '"':
            break;
        default:
            ss << c;
            break;
        }
    }
    assert(type.size() == 1);
    return ss.str();
}

std::string positionToHtml(const vts::Position &pos)
{
    std::stringstream ss;
    ss << "<table>" << std::fixed;
    ss << std::setprecision(10);
    for (int i = 0; i < 3; i++)
        ss << "<tr><td>Pos " << ("XYZ"[i])
           << "<td class=number>" << pos.point[i] << "</tr>";
    ss << std::setprecision(5);
    for (int i = 0; i < 3; i++)
        ss << "<tr><td>Rot " << (char)('X' + i)
           << "<td class=number>" << pos.orientation[i] << "</tr>";
    ss << "<tr><td>Extent<td class=number>"
       << std::setprecision(3) << pos.viewExtent << "</tr>";
    ss << "<tr><td>Fov<td class=number>"
       << std::setprecision(1) << pos.fov << "</tr>";
    ss << "</table>";
    return ss.str();
}

