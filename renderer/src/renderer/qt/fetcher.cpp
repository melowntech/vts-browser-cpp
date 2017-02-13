#include "fetcher.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QAuthenticator>

namespace
{
    class FetcherDetail : public QNetworkAccessManager
    {
    public:
        FetcherDetail()
        {
            connect(this, &QNetworkAccessManager::authenticationRequired, this, &FetcherDetail::authentication);
        }

        void authentication(QNetworkReply *, QAuthenticator *authenticator)
        {
            authenticator->setUser(QString::fromUtf8(options.username.data(), options.username.size()));
            authenticator->setPassword(QString::fromUtf8(options.password.data(), options.password.size()));
        }

        void fetch(melown::FetchType type, const std::string &name);

        FetcherOptions options;
        FetcherImpl::Func func;
    };

    class FetchTask : public QObject
    {
    public:
        FetchTask(FetcherDetail *fetcher, melown::FetchType type, const std::string &name) : name(name), fetcher(fetcher), reply(nullptr), type(type), redirections(0)
        {}

        void finished()
        {
            reply->deleteLater();

            if (reply->error() != QNetworkReply::NoError)
            {
                fetcher->func(type, name, nullptr, 0);
                delete this;
                return;
            }

            QVariant redirVar = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
            QUrl redir = redirVar.toUrl();
            if (!redir.isEmpty() && redir != reply->request().url())
            { // do redirect
                if (redirections++ > 5)
                { // too many redirections
                    fetcher->func(type, name, nullptr, 0);
                    delete this;
                    return;
                }

                reply = fetcher->get(QNetworkRequest(redir));
                connect(reply, &QNetworkReply::finished, this, &FetchTask::finished);
                return;
            }

            QByteArray arr = reply->readAll();
            fetcher->func(type, name, arr.data(), arr.size());
            delete this;
        }

        const std::string name;
        FetcherDetail *fetcher;
        QNetworkReply *reply;
        const melown::FetchType type;
        melown::uint32 redirections;
    };

    void FetcherDetail::fetch(melown::FetchType type, const std::string &name)
    {
        FetchTask *t = new FetchTask(this, type, name);
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

void FetcherImpl::setOptions(const FetcherOptions &options)
{
    impl->options = options;
}

void FetcherImpl::setCallback(Func func)
{
    impl->func = func;
}

void FetcherImpl::fetch(melown::FetchType type, const std::string &name)
{
    return impl->fetch(type, name);
}
