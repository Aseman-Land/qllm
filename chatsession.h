#ifndef CHATSESSION_H
#define CHATSESSION_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "chatsmodel.h"

class ChatSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint32 currentChat READ currentChat WRITE setCurrentChat NOTIFY currentChatChanged FINAL)
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged FINAL)
    Q_PROPERTY(QString autoAnswerModel READ autoAnswerModel WRITE setAutoAnswerModel NOTIFY autoAnswerModelChanged FINAL)

public:
    struct Message {
        qint32 id = 0;
        QString model;
        QString role;
        QString content;
        QDateTime datetime;
    };
    typedef QSharedPointer<Message> MessagePtr;

    ChatSession(ChatsModel *model, QObject *parent = nullptr);
    virtual ~ChatSession();

    qint32 currentChat() const;
    void setCurrentChat(qint32 newCurrentChat);

    QList<MessagePtr> messages() const;

    QString baseUrl() const;
    void setBaseUrl(const QString &newBaseUrl);

    QString autoAnswerModel() const;
    void setAutoAnswerModel(const QString &newAutoAnswerModel);

    bool deleteMessage(MessagePtr ptr);

public Q_SLOTS:
    void reload();
    void sendPrompt(const QString &model, const QString &prompt);

Q_SIGNALS:
    void currentChatChanged();
    void messagesChanged();
    void baseUrlChanged();
    void autoAnswerModelChanged();

protected:
    void store(const MessagePtr &ptr);
    void sendPrompt(const QString &model, const QString &prompt, bool human, bool isAutoSend);

private:
    ChatsModel *mModel;

    qint32 mCurrentChat = 0;
    QString mBaseUrl;

    QString mAutoAnswerModel;

    QNetworkAccessManager *mAm;
    QNetworkReply *mActiveReply = nullptr;
    QHash<QString, MessagePtr> mActiveReplyMessages;

    QList<MessagePtr> mMessages;
};

#endif // CHATSESSION_H
