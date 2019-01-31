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

#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <vts-browser/buffer.hpp>
#include <vts-browser/log.hpp>

#include "editor.hpp"

namespace
{
    const std::string tmpName = "vts-browser-editing.json";

    bool runSystemCommand(const std::string &cmd)
    {
        vts::log(vts::LogLevel::info1, std::string()
                 + "Running command: <" + cmd + ">");
        auto res = std::system(cmd.c_str());
        std::ostringstream ss;
        ss << "Result of System call: " << res;
        vts::log(vts::LogLevel::info1, ss.str());
        return res == 0;
    }
}

std::string editor(const std::string &name, const std::string &value)
{
    vts::log(vts::LogLevel::info3,
             std::string() + "Editing file <" + name + ">");

    std::string result = value;

    struct Deleter
    {
        std::string name;
        Deleter(const std::string &name) : name(name) {}
        ~Deleter() { try { std::remove(name.c_str()); } catch(...) {} }
    } deleter(tmpName);

    try
    {
        vts::writeLocalFileBuffer(tmpName, vts::Buffer(value));
#ifdef _WIN32
        if (getenv("EDITOR"))
            runSystemCommand(std::string() + "%EDITOR% " + tmpName);
        else
        {
            if (!runSystemCommand(std::string() + "notepad++.exe -multiInst -notabbar -nosession -noPlugin " + tmpName))
                runSystemCommand(std::string() + "notepad.exe " + tmpName);
        }
#else
        if (getenv("EDITOR"))
            runSystemCommand(std::string() + "xterm -e $EDITOR " + tmpName);
        else
        {
            if (!runSystemCommand(std::string() + "gedit " + tmpName))
                if (!runSystemCommand(std::string() + "xterm -e nano " + tmpName))
                    runSystemCommand(std::string() + "xterm -e vim " + tmpName);
        }
#endif // _WIN32
        result = vts::readLocalFileBuffer(tmpName).str();
    }
    catch (...)
    {
        vts::log(vts::LogLevel::warn3,
            std::string() + "Failed editing file <" + name + ">");
    }

    return result;
}

