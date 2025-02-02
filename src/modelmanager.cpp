#include "modelmanager.h"

#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QCoreApplication>

ModelManagerItem::ModelManagerItem(const QString &name, QObject *parent)
    : QObject{parent}
    , mName(name)
{
    mAm = new QNetworkAccessManager(this);
}

ModelManagerItem::~ModelManagerItem()
{
    stop();
}

void ModelManagerItem::start()
{
    if (mActiveReply)
        return;

    QUrl url(mBaseUrl + "/pull");

    auto buffer = new QByteArray;

    QNetworkRequest req;
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setUrl(url);

    QJsonObject obj;
    obj["name"] = mName;

    auto reply = mAm->post(req, QJsonDocument(obj).toJson(QJsonDocument::Compact));

    mActiveReply = reply;
    connect(reply, &QNetworkReply::readyRead, this, [this, reply, buffer](){
        const auto data = reply->readAll();
        buffer->append(data);
        while (buffer->contains('\n'))
        {
            const auto idx = buffer->indexOf('\n');
            const auto data = buffer->left(idx+1);
            buffer->remove(0, idx+1);

            const auto json = QJsonDocument::fromJson(data);
            if (!json.isObject())
            {
                qDebug() << "invalid data:" << data;
                return;
            }

            const auto obj = json.object();
            const auto status = obj.value("status").toString();
            const auto total = obj.value("total").toVariant().toLongLong();
            const auto completed = obj.value("completed").toVariant().toLongLong();

            setStatus(status);
            setTotal(total);
            setDownloaded(completed);
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        reply->deleteLater();
        if (reply == mActiveReply)
            mActiveReply = nullptr;
    });
    connect(reply, &QNetworkReply::destroyed, this, [this, reply, buffer](){
        delete buffer;
        Q_EMIT downloadingChanged();
    });

    Q_EMIT downloadingChanged();
}

void ModelManagerItem::stop()
{
    if (!mActiveReply)
        return;

    mActiveReply->close();
    mActiveReply->deleteLater();
    mActiveReply = nullptr;
    Q_EMIT cancelRequest();
}

qint64 ModelManagerItem::total() const
{
    return mTotal;
}

bool ModelManagerItem::downloading() const
{
    return mActiveReply;
}

void ModelManagerItem::setTotal(qint64 newTotal)
{
    if (mTotal == newTotal)
        return;
    mTotal = newTotal;
    Q_EMIT totalChanged();
}

qint64 ModelManagerItem::downloaded() const
{
    return mDownloaded;
}

void ModelManagerItem::setDownloaded(qint64 newDownloaded)
{
    if (mDownloaded == newDownloaded)
        return;
    mDownloaded = newDownloaded;
    Q_EMIT downloadedChanged();
}

QString ModelManagerItem::status() const
{
    return mStatus;
}

void ModelManagerItem::setStatus(const QString &newStatus)
{
    if (mStatus == newStatus)
        return;
    mStatus = newStatus;
    Q_EMIT statusChanged();
}

QString ModelManagerItem::name() const
{
    return mName;
}

QString ModelManagerItem::baseUrl() const
{
    return mBaseUrl;
}

void ModelManagerItem::setBaseUrl(const QString &newBaseUrl)
{
    if (mBaseUrl == newBaseUrl)
        return;
    mBaseUrl = newBaseUrl;
    Q_EMIT baseUrlChanged();
}



ModelManager::ModelManager(QObject *parent)
    : QObject{parent}
{
    mAm = new QNetworkAccessManager(this);
}

ModelManager::~ModelManager()
{
}

ModelManagerItem *ModelManager::pull(const QString &name)
{
    if (mItems.contains(name))
        return mItems.value(name);

    auto item = new ModelManagerItem(name, this);
    item->setBaseUrl(mBaseUrl);
    item->start();

    connect(item, &ModelManagerItem::cancelRequest, this, [name, this, item](){
        mItems.remove(name);
        item->deleteLater();
        Q_EMIT itemsChanged();
    });

    mItems[name] = item;
    Q_EMIT itemsChanged();
    return item;
}

void ModelManager::remove(const QString &name)
{
    QUrl url(mBaseUrl + "/delete");

    QNetworkRequest req;
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setUrl(url);

    QJsonObject obj;
    obj["name"] = name;

    auto reply = mAm->post(req, QJsonDocument(obj).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        reload();
        reply->deleteLater();
    });
}

void ModelManager::reload()
{
    QUrl url(mBaseUrl + "/tags");

    QNetworkRequest req;
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setUrl(url);

    auto reply = mAm->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        const auto data = reply->readAll();
        const auto json = QJsonDocument::fromJson(data);
        reply->deleteLater();

        if (!json.isObject())
        {
            qDebug() << "invalid data:" << data;
            return;
        }

        const auto root = json.object();
        const auto models = root.value("models");

        const auto count = mItems.count();
        for (const auto &item: models.toArray())
        {
            if (!item.isObject())
                continue;

            const auto obj = item.toObject();
            const auto model = obj.value("model").toString();
            if (!mItems.contains(model))
                mItems[model] = new ModelManagerItem(model, this);
        }
        if (mItems.count() != count)
            Q_EMIT itemsChanged();
    });
}

QList<ModelManagerItem *> ModelManager::items() const
{
    return mItems.values();
}

QString ModelManager::baseUrl() const
{
    return mBaseUrl;
}

void ModelManager::setBaseUrl(const QString &newBaseUrl)
{
    if (mBaseUrl == newBaseUrl)
        return;
    mBaseUrl = newBaseUrl;
    for (const auto &item: mItems)
        item->setBaseUrl(item->baseUrl());
    reload();
    Q_EMIT baseUrlChanged();
}
