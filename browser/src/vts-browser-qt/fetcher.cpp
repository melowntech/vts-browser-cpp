#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QAuthenticator>
#include "fetcher.hpp"

class FetcherDetail : public QNetworkAccessManager
{
public:
    void fetch(std::shared_ptr<vts::FetchTask> task);

    FetcherImpl::Func func;
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
            task->replyCode = 404;
        else
        {
            QVariant redirVar = reply->attribute
                    (QNetworkRequest::RedirectionTargetAttribute);
            QUrl redir = redirVar.toUrl();
            if (!redir.isEmpty() && redir != reply->request().url())
            { // do redirect
                task->replyCode = 302;
                task->replyRedirectUrl = (char*)redir.toString().unicode();
            }
            else
            {
                task->replyCode = 200;
                QByteArray arr = reply->readAll();
                task->contentData.allocate(arr.size());
                memcpy(task->contentData.data(), arr.data(), arr.size());
            }
        }

        fetcher->func(task);
        delete this;
    }

    std::shared_ptr<vts::FetchTask> task;
    FetcherDetail *fetcher;
    QNetworkReply *reply;
};

void FetcherDetail::fetch(std::shared_ptr<vts::FetchTask> task)
{
    FetchTask *t = new FetchTask(this, task);
    QUrl url(QString::fromUtf8(t->task->queryUrl.data(),
                               t->task->queryUrl.length()));
    QNetworkRequest request(url);
    for (auto it : task->queryHeaders)
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

void FetcherImpl::initialize(Func func)
{
    impl->func = func;
}

void FetcherImpl::finalize()
{}

void FetcherImpl::fetch(std::shared_ptr<vts::FetchTask> task)
{
    return impl->fetch(task);
}
