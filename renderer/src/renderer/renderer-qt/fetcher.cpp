#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QAuthenticator>

#include "fetcher.h"

namespace
{

class FetcherDetail : public QNetworkAccessManager
{
public:
    FetcherDetail()
    {
        connect(this, &QNetworkAccessManager::authenticationRequired,
                this, &FetcherDetail::authentication);
    }

    void authentication(QNetworkReply *, QAuthenticator *authenticator)
    {
        authenticator->setUser(QString::fromUtf8(options.username.data(),
                                                 options.username.size()));
        authenticator->setPassword(QString::fromUtf8(options.password.data(),
                                                     options.password.size()));
    }

    void fetch(melown::FetchTask *task);

    FetcherOptions options;
    FetcherImpl::Func func;
};

class FetchTask : public QObject
{
public:
    FetchTask(FetcherDetail *fetcher, melown::FetchTask *task) : task(task),
        fetcher(fetcher), reply(nullptr)
    {}

    void finished()
    {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
            task->code = 404;
        else
        {
            QVariant redirVar = reply->attribute
                    (QNetworkRequest::RedirectionTargetAttribute);
            QUrl redir = redirVar.toUrl();
            if (!redir.isEmpty() && redir != reply->request().url())
            { // do redirect
                task->code = 302;
                task->redirectUrl = (char*)redir.toString().unicode();
            }
            else
            {
                task->code = 200;
                QByteArray arr = reply->readAll();
                task->contentData.allocate(arr.size());
                memcpy(task->contentData.data, arr.data(), arr.size());
            }
        }

        fetcher->func(task);
        delete this;
    }

    melown::FetchTask *task;
    FetcherDetail *fetcher;
    QNetworkReply *reply;
};

void FetcherDetail::fetch(melown::FetchTask *task)
{
    FetchTask *t = new FetchTask(this, task);
    QUrl url(QString::fromUtf8(t->task->url.data(), t->task->url.length()));
    QNetworkRequest request(url);
    t->reply = get(request);
    connect(t->reply, &QNetworkReply::finished, t, &FetchTask::finished);
}

} // namespace

FetcherImpl::FetcherImpl() : impl(nullptr)
{
    impl = new FetcherDetail();
}

FetcherImpl::~FetcherImpl()
{
    delete impl;
}

void FetcherImpl::setOptions(const FetcherOptions &options)
{
    impl->options = options;
}

void FetcherImpl::initialize(Func func)
{
    impl->func = func;
}

void FetcherImpl::finalize()
{}

void FetcherImpl::fetch(melown::FetchTask *task)
{
    return impl->fetch(task);
}
