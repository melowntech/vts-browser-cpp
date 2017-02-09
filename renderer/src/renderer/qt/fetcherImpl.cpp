#include "fetcherImpl.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

namespace
{
    class FetcherDetail : public QNetworkAccessManager
    {
    public:
        void fetch(const std::string &name);

        FetcherImpl::Func func;
    };

    class FetchTask : public QObject
    {
    public:
        FetchTask(FetcherDetail *fetcher, const std::string &name) : name(name), fetcher(fetcher), reply(nullptr), redirections(0)
        {}

        void finished()
        {
            reply->deleteLater();
            qDebug() << "fetch: " << name.data() << ", finished: " << reply->request().url() << ", status: " << reply->errorString();

            QVariant redirVar = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
            QUrl redir = redirVar.toUrl();
            if (!redir.isEmpty() && redir != reply->request().url())
            { // do redirect
                if (redirections++ > 5)
                { // too many redirections
                    qDebug() << "too many redirections";
                    fetcher->func(name, nullptr, 0);
                    delete this;
                    return;
                }

                reply = fetcher->get(QNetworkRequest(redir));
                connect(reply, &QNetworkReply::finished, this, &FetchTask::finished);
                return;
            }

            QByteArray arr = reply->readAll();
            fetcher->func(name, arr.data(), arr.size());
            delete this;
        }

        const std::string name;
        FetcherDetail *fetcher;
        QNetworkReply *reply;
        melown::uint32 redirections;
    };

    void FetcherDetail::fetch(const std::string &name)
    {
        FetchTask *t = new FetchTask(this, name);
        QUrl url(QString::fromUtf8(name.data(), name.length()));
        QNetworkRequest request(url);
        t->reply = get(request);
        connect(t->reply, &QNetworkReply::finished, t, &FetchTask::finished);
    }
}

FetcherImpl::FetcherImpl() : impl(nullptr)
{
    impl = new FetcherDetail();
}

FetcherImpl::~FetcherImpl()
{
    delete impl;
}

void FetcherImpl::setCallback(Func func)
{
    impl->func = func;
}

void FetcherImpl::fetch(const std::string &name)
{
    return impl->fetch(name);
}
