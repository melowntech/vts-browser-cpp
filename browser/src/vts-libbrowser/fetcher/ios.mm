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

#import <Foundation/Foundation.h>

namespace vts
{

namespace
{

class FetcherImpl : public Fetcher
{
public:
    NSURLSession *session;

    void completed(const std::shared_ptr<FetchTask> &task, NSData *data, NSHTTPURLResponse *response)
    {
        task->reply.contentType = [response.MIMEType UTF8String];
        task->reply.code = response.statusCode;
        task->reply.expires = -2;
        task->reply.content.allocate([data length]);
        memcpy(task->reply.content.data(), [data bytes], [data length]);
    }

    virtual void fetch(const std::shared_ptr<FetchTask> &task_)
    {
        std::shared_ptr<FetchTask> task = task_;
        NSString *urlString = [NSString stringWithCString:task->query.url.c_str() encoding:NSUTF8StringEncoding];
        urlString = [urlString stringByAddingPercentEncodingWithAllowedCharacters: NSCharacterSet.URLQueryAllowedCharacterSet];
            NSURL *url = [NSURL URLWithString:urlString];
        [[session dataTaskWithURL:url completionHandler:^(NSData *data, NSURLResponse *response, NSError *error)
        {
            if (error)
            {
                task->reply.code = FetchTask::ExtraCodes::InternalError;
            }
            else
            {
                assert([response isKindOfClass: [NSHTTPURLResponse class]]);
                completed(task, data, (NSHTTPURLResponse*)response);
            }
            task->fetchDone();
        }] resume];
    }

    FetcherImpl(const FetcherOptions &options)
    {
        NSURLSessionConfiguration *config = [NSURLSessionConfiguration defaultSessionConfiguration];
        session = [NSURLSession sessionWithConfiguration:config];
        [session retain];
    }

    ~FetcherImpl()
    {
        [session invalidateAndCancel];
        [session release];
    }
};

}

std::shared_ptr<Fetcher> Fetcher::create(const FetcherOptions &options)
{
    return std::dynamic_pointer_cast<Fetcher>(
                std::make_shared<FetcherImpl>(options));
}

} // namespace vts

