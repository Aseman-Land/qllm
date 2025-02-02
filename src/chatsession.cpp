#include "chatsession.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QTimer>
#include <QDebug>

ChatSession::ChatSession(ChatsModel *model, QObject *parent)
    : QObject{parent}
    , mModel(model)
{
    mAm = new QNetworkAccessManager(this);
}

ChatSession::~ChatSession()
{
}

qint32 ChatSession::currentChat() const
{
    return mCurrentChat;
}

void ChatSession::setCurrentChat(qint32 newCurrentChat)
{
    if (mCurrentChat == newCurrentChat)
        return;
    if (mCurrentChat)
        setAutoAnswerModel(QString());
    mCurrentChat = newCurrentChat;
    reload();
    Q_EMIT currentChatChanged();
}

void ChatSession::reload()
{
    mMessages.clear();

    auto db = QSqlDatabase::database(mModel->dbConnection());
    QSqlQuery q(db);
    q.prepare("SELECT * FROM messages WHERE chat_id=:chat_id");
    q.bindValue(":chat_id", mCurrentChat);
    if (q.exec())
    {
        while (q.next())
        {
            const auto r = q.record();
            auto msg = MessagePtr::create();
            msg->id = r.value("id").toInt();
            msg->model = r.value("model").toString();
            msg->role = r.value("role").toString();
            msg->content = r.value("content").toString();
            msg->datetime = QDateTime::fromMSecsSinceEpoch(r.value("datetime").toLongLong());

            mMessages.append(msg);
        }
    }
    else
        qDebug() << q.lastError();

    Q_EMIT messagesChanged();
}

void ChatSession::sendPrompt(const QString &model, const QString &prompt)
{
    sendPrompt(model, prompt, true, false);
}

void ChatSession::sendPrompt(const QString &model, const QString &prompt, bool human, bool isAutoSend)
{
    mModel->dbBegin();

    if (mCurrentChat == 0)
        setCurrentChat(mModel->create(prompt.left(64)));

    QUrl url(mBaseUrl + "/chat");

    auto buffer = new QByteArray;

    QNetworkRequest req;
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setUrl(url);

    auto promptMsg = MessagePtr::create();
    promptMsg->datetime = QDateTime::currentDateTime();
    promptMsg->content = prompt;
    promptMsg->role = "user";

    if (mAutoAnswerModel.count())
        promptMsg->model = isAutoSend? mAutoAnswerModel : model;
    else
        promptMsg->model = model;

    if (human)
    {
        mMessages << promptMsg;
        store(promptMsg);
    }

    QJsonArray chat;
    for (const auto &msg: mMessages)
    {
        QJsonObject c;
        if (!human)
            c["role"] = (msg->role == "user"? "assistant" : msg->role == "user"? "assistant" : "system");
        else
            c["role"] = msg->role;
        c["content"] = msg->content;
        chat << c;
    }

    QJsonObject obj;
    obj["model"] = promptMsg->model;
    obj["messages"] = chat;

    auto reply = mAm->post(req, QJsonDocument(obj).toJson(QJsonDocument::Compact));

    mActiveReply = reply;
    mActiveReplyMessages.clear();

    const auto readData = [this, reply, buffer](bool finalPart){
        buffer->append(reply->readAll());
        while (buffer->contains('\n') || finalPart)
        {
            const auto idx = buffer->indexOf('\n');
            const auto data = finalPart? *buffer : buffer->left(idx+1);
            buffer->remove(0, idx+1);

            if (data.isEmpty())
                return;

            const auto json = QJsonDocument::fromJson(data);
            if (!json.isObject())
            {
                qDebug() << "invalid data:" << data;
                return;
            }

            const auto obj = json.object();
            const auto model = obj.value("model").toString();
            const auto done = obj.value("done").toBool();
            const auto message = obj.value("message").toObject();

            const auto role = message.value("role").toString();
            const auto content = message.value("content").toString();
            if (content.isEmpty())
                continue;

            if (reply == mActiveReply)
            {
                MessagePtr &responceMsg = mActiveReplyMessages[role];
                if (!responceMsg)
                {
                    responceMsg = MessagePtr::create();
                    responceMsg->datetime = QDateTime::currentDateTime();
                    responceMsg->role = role;
                    responceMsg->model = model;

                    mMessages.append(responceMsg);
                }

                responceMsg->content += content;
            }

            Q_EMIT messagesChanged();
            if (finalPart)
                break;
        }
    };

    connect(mActiveReply, &QNetworkReply::readyRead, this, [readData](){ readData(false); });
    connect(mActiveReply, &QNetworkReply::finished, this, [this, reply, buffer, isAutoSend, model, readData](){
        readData(true);
        reply->deleteLater();
        delete buffer;
        if (reply == mActiveReply)
        {
            if (mAutoAnswerModel.count() && mActiveReplyMessages.contains("assistant"))
            {
                const auto msg = mActiveReplyMessages.value("assistant");
                const auto content = msg->content;
                QTimer::singleShot(500, this, [this, model, content, isAutoSend](){
                    if (isAutoSend)
                        sendPrompt(model, content, false, false);
                    else
                        sendPrompt(model, content, false, true);
                });
            }

            for (const auto &msg: mActiveReplyMessages)
                store(msg);

            mActiveReplyMessages.clear();
            mActiveReply = nullptr;
            Q_EMIT messagesChanged();
        }
    });

    Q_EMIT messagesChanged();
}

void ChatSession::store(const MessagePtr &msg)
{
    mModel->dbBegin();

    auto db = QSqlDatabase::database(mModel->dbConnection());
    QSqlQuery q(db);
    q.prepare(msg->id? "INSERT OR REPLACE INTO messages (id, model, role, chat_id, content, datetime) VALUES (:id, :model, :role, :chat_id, :content, :datetime)"
                   : "INSERT OR REPLACE INTO messages (model, role, chat_id, content, datetime) VALUES (:model, :role, :chat_id, :content, :datetime)");
    q.bindValue(":id", msg->id);
    q.bindValue(":model", msg->model);
    q.bindValue(":role", msg->role);
    q.bindValue(":chat_id", mCurrentChat);
    q.bindValue(":content", msg->content);
    q.bindValue(":datetime", msg->datetime.toMSecsSinceEpoch());
    if (!q.exec())
    {
        qDebug() << q.lastError();
        return;
    }

    msg->id = q.lastInsertId().toInt();
}

QString ChatSession::autoAnswerModel() const
{
    return mAutoAnswerModel;
}

void ChatSession::setAutoAnswerModel(const QString &newAutoAnswerModel)
{
    if (mAutoAnswerModel == newAutoAnswerModel)
        return;
    mAutoAnswerModel = newAutoAnswerModel;
    Q_EMIT autoAnswerModelChanged();
}

bool ChatSession::deleteMessage(MessagePtr msg)
{
    mModel->dbBegin();

    auto db = QSqlDatabase::database(mModel->dbConnection());
    QSqlQuery q(db);
    q.prepare("DELETE FROM messages WHERE id = :id");
    q.bindValue(":id", msg->id);
    if (!q.exec())
    {
        qDebug() << q.lastError();
        return false;
    }
    reload();
    return true;
}

QString ChatSession::baseUrl() const
{
    return mBaseUrl;
}

void ChatSession::setBaseUrl(const QString &newBaseUrl)
{
    if (mBaseUrl == newBaseUrl)
        return;
    mBaseUrl = newBaseUrl;
    Q_EMIT baseUrlChanged();
}

QList<ChatSession::MessagePtr> ChatSession::messages() const
{
    return mMessages;
}
