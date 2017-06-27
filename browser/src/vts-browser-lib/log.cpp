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

#include <sys/prctl.h>
#include <dbglog/dbglog.hpp>
#include "include/vts-browser/log.hpp"

namespace vts
{

void setLogMask(const std::string &mask)
{
    dbglog::set_mask(mask);
}

void setLogMask(LogLevel mask)
{
    dbglog::set_mask((dbglog::level)mask);
}

void setLogConsole(bool enable)
{
    dbglog::log_console(enable);
}

void setLogFile(const std::string &filename)
{
    dbglog::log_file(filename);
}

void setLogThreadName(const std::string &name)
{
    dbglog::thread_id(name);
    prctl(PR_SET_NAME,name.c_str(),0,0,0);
}

namespace
{

class LogSink : public dbglog::Sink
{
public:
    LogSink(LogLevel mask, std::function<void(const std::string&)> callback)
        : Sink((dbglog::level)mask, "app log sink"), callback(callback)
    {}
    
    virtual void write(const std::string &line)
    {
        callback(line);
    }
    
    std::function<void(const std::string&)> callback;
};

} // namespace

void addLogSink(LogLevel mask, std::function<void(const std::string&)> callback)
{
    auto s = boost::shared_ptr<LogSink>(new LogSink(mask, callback));
    dbglog::add_sink(s);
}

void log(LogLevel level, const std::string &message)
{
    LOGR((dbglog::level)level) << message;
}

} // namespace vts
