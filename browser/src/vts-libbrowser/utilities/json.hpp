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

#ifndef JSON_HPP_sgf56489dh4d69
#define JSON_HPP_sgf56489dh4d69

#include <json/json.h>
#include <sstream>

namespace vts
{

Json::Value stringToJson(const std::string &s);
std::string jsonToString(const Json::Value &value);

// json to enum
template<class T>
T jToE(const Json::Value &j)
{
    std::string s = j.asString();
    T e;
    std::istringstream ss(s);
    ss >> e;
    return e;
}

// enum to json
template<class T>
Json::Value eToJ(T e)
{
    std::ostringstream ss;
    ss << e;
    std::string s = ss.str();
    return s;
}

} // namespace vts

#define TJ(NAME, AS) v[#NAME] = NAME ;
#define AJ(NAME, AS) if (v.isMember(#NAME)) NAME = v[#NAME].AS();
#define TJE(NAME, TYPE) v[#NAME] = eToJ<TYPE>(NAME);
#define AJE(NAME, TYPE) if (v.isMember(#NAME)) { NAME = jToE<TYPE>(v[#NAME]); }

#endif
