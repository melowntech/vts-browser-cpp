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

#include "../include/vts-browser/fetcher.hpp"

#include <Windows.Foundation.h>
#include <Windows.Web.Http.Headers.h>
#include <ppltasks.h>

#include <stdexcept>
#include <vector>

using namespace Platform;
using namespace Concurrency;
using namespace Windows;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Web;
using namespace Windows::Web::Http;
using namespace Windows::Security::Cryptography;

/*
std::wstring widen(const std::string &str)
{
    if (str.empty())
        return {};
    size_t charsNeeded = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), NULL, 0);
    std::wstring buffer;
    buffer.resize(charsNeeded);
    ::MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), buffer.data(), buffer.size());
    return buffer;
}

std::string narrow(const std::wstring &str)
{
    if (str.empty())
        return {};
    size_t charsNeeded = ::WideCharToMultiByte(CP_UTF8, 0, str.data(), str.size(), NULL, 0, 0, 0);
    std::string buffer;
    buffer.resize(charsNeeded);
    ::WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), buffer.data(), buffer.size(), 0, 0);
    return buffer;
}
*/

String ^widen(const std::string &str)
{
    if (str.empty())
        return {};
    size_t charsNeeded = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), NULL, 0);
    std::wstring buffer;
    buffer.resize(charsNeeded);
    ::MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), &buffer[0], buffer.size());
    return ref new String(buffer.data());
}

std::string narrow(String ^str)
{
    if (str->Length() == 0)
        return {};
    size_t charsNeeded = ::WideCharToMultiByte(CP_UTF8, 0, str->Data(), str->Length(), NULL, 0, 0, 0);
    std::string buffer;
    buffer.resize(charsNeeded);
    ::WideCharToMultiByte(CP_UTF8, 0, str->Data(), str->Length(), &buffer[0], buffer.size(), 0, 0);
    return buffer;
}

namespace vts
{

namespace
{

class FetcherImpl : public Fetcher
{
    HttpClient ^client;

public:
    FetcherImpl(const FetcherOptions &options)
    {}

    ~FetcherImpl()
    {}

    void initialize() override
    {
        client = ref new HttpClient();
    }

    void finalize() override
    {}

    void fetch(const std::shared_ptr<FetchTask> &task_) override
    {
        const std::shared_ptr<FetchTask> task = task_;
        try
        {
            //auto headers{ client->DefaultRequestHeaders() };
            //for (auto it : task->query.headers)
            //    headers.Append(widen(it.first), widen(it.second));

            Uri ^requestUri = ref new Uri(widen(task->query.url));
            create_task(client->GetAsync(requestUri))
                .then([=](HttpResponseMessage ^response) {
                    task->reply.code = (uint32)response->StatusCode;
                    IBuffer ^body = response->Content->ReadAsBufferAsync()->GetResults();
                    Array<unsigned char> ^array = nullptr;
                    CryptographicBuffer::CopyToByteArray(body, &array);
                    task->reply.content.allocate(array->Length);
                    memcpy(task->reply.content.data(), array->Data, array->Length);
                })
                .then([=](Concurrency::task<void> t)
                {
                    try
                    {
                        t.get();
                    }
                    catch (...)
                    {
                        task->reply.code = FetchTask::ExtraCodes::InternalError;
                    }
                    task->fetchDone();
                });
        }
        catch (...)
        {
            task->reply.code = FetchTask::ExtraCodes::InternalError;
            task->fetchDone();
        }
    }
};

} // namespace

std::shared_ptr<Fetcher> Fetcher::create(const FetcherOptions &options)
{
    return std::dynamic_pointer_cast<Fetcher>(
        std::make_shared<FetcherImpl>(options));
}

} // namespace vts
