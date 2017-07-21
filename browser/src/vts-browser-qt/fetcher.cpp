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

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QAuthenticator>
#include "fetcher.hpp"

class FetcherDetail : public QNetworkAccessManager
{
public:
    void fetch(const std::shared_ptr<vts::FetchTask> &task);
};

class FetchTask : public QObject
{
public:
    FetchTask(FetcherDetail *fetcher, std::shared_ptr<vts::FetchTask> task)
        : task(task), fetcher(fetcher), reply(nullptr)
    {}

    void finished()
    {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
            task->reply.code = 404;
        else
        {
            QVariant redirVar = reply->attribute
                    (QNetworkRequest::RedirectionTargetAttribute);
            QUrl redir = redirVar.toUrl();
            if (!redir.isEmpty() && redir != reply->request().url())
            {
                // do redirect
                task->reply.code = 302;
                task->reply.redirectUrl = (char*)redir.toString().unicode();
            }
            else
            {
                task->reply.code = 200;
                QByteArray arr = reply->readAll();
                task->reply.content.allocate(arr.size());
                memcpy(task->reply.content.data(), arr.data(), arr.size());
            }
        }

        task->fetchDone();
        delete this;
    }

    std::shared_ptr<vts::FetchTask> task;
    FetcherDetail *fetcher;
    QNetworkReply *reply;
};

void FetcherDetail::fetch(const std::shared_ptr<vts::FetchTask> &task)
{
    FetchTask *t = new FetchTask(this, task);
    QUrl url(QString::fromUtf8(t->task->query.url.data(),
                               t->task->query.url.length()));
    QNetworkRequest request(url);
    for (auto it : task->query.headers)
    {
        request.setRawHeader(QByteArray(it.first.c_str(), it.first.length()),
                             QByteArray(it.second.c_str(), it.second.length()));
    }
    t->reply = get(request);
    connect(t->reply, &QNetworkReply::finished, t, &FetchTask::finished);
}

FetcherImpl::FetcherImpl() : impl(nullptr)
{
    impl = std::make_shared<FetcherDetail>();
}

FetcherImpl::~FetcherImpl()
{}

void FetcherImpl::initialize()
{}

void FetcherImpl::finalize()
{}

void FetcherImpl::fetch(const std::shared_ptr<vts::FetchTask> &task)
{
    return impl->fetch(task);
}
