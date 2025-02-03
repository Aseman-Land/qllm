#include "chatsmodel.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QCryptographicHash>
#include <QDebug>
#include <QIcon>

ChatsModel::ChatsModel(QObject *parent)
    : QAbstractItemModel{parent}
{
    mDbCommitTimer = new QTimer(this);
    mDbCommitTimer->setInterval(300);

    connect(mDbCommitTimer, &QTimer::timeout, this, &ChatsModel::dbCommit);
}

ChatsModel::~ChatsModel()
{
    destroyDatabase();
}

int ChatsModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return mChats.count();
    return 0;
}

int ChatsModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant ChatsModel::data(const QModelIndex &index, int role) const
{
    const auto chat = static_cast<Chat*>(index.internalPointer());
    if (!chat)
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
        return chat->name;
    case Qt::DecorationRole:
        return QIcon(":/ui/icons/icon.svg");
    }

    return QVariant();
}

QModelIndex ChatsModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

QModelIndex ChatsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row >= mChats.count())
        return QModelIndex();

    return createIndex(row, column, mChatsHash.value(mChats.at(row)).data());
}

QModelIndex ChatsModel::indexOf(qint32 chatId) const
{
    const auto row = mChats.indexOf(chatId);
    if (row < 0)
        return QModelIndex();
    return index(row, 0);
}

qint32 ChatsModel::chatId(QModelIndex index) const
{
    const auto chat = static_cast<Chat*>(index.internalPointer());
    if (!chat)
        return 0;
    return chat->id;
}

QString ChatsModel::fileLocation() const
{
    return mFileLocation;
}

void ChatsModel::setFileLocation(const QString &newFileLocation)
{
    if (mFileLocation == newFileLocation)
        return;

    mFileLocation = newFileLocation;

    if (mDbConnection.size())
        destroyDatabase();

    mDbConnection = mFileLocation.isEmpty()? QString() : QString::fromLatin1(QCryptographicHash::hash(mFileLocation.toUtf8(), QCryptographicHash::Sha256).toBase64());
    if (mDbConnection.size())
        initDatabase();

    reload();
    Q_EMIT fileLocationChanged();
}

QString ChatsModel::dbConnection() const
{
    return mDbConnection;
}

void ChatsModel::dbBegin(bool autoCommit)
{
    if (mDbConnection.isEmpty())
        return;
    if (mDbCommitTimer->isActive())
        return;

    auto db = QSqlDatabase::database(mDbConnection);
    QSqlQuery q(db);
    q.prepare("BEGIN");
    q.exec();

    mDbCommitTimer->start();
}

void ChatsModel::dbCommit()
{
    if (mDbConnection.isEmpty())
        return;
    if (!mDbCommitTimer->isActive())
        return;

    auto db = QSqlDatabase::database(mDbConnection);
    QSqlQuery q(db);
    q.prepare("COMMIT");
    q.exec();

    mDbCommitTimer->stop();
}

void ChatsModel::reload()
{
    beginResetModel();
    mChats.clear();
    mChatsHash.clear();

    auto db = QSqlDatabase::database(mDbConnection);
    QSqlQuery q(db);
    q.prepare("SELECT * FROM chats");
    if (q.exec())
    {
        while (q.next())
        {
            const auto r = q.record();
            auto c = ChatPtr::create();
            c->id = r.value("id").toInt();
            c->name = r.value("name").toString();
            c->datetime = QDateTime::fromMSecsSinceEpoch(r.value("datetime").toLongLong());

            mChats.prepend(c->id);
            mChatsHash[c->id] = c;
        }
    }
    else
        qDebug() << q.lastError();

    endResetModel();
}

qint32 ChatsModel::create(const QString &name)
{
    dbBegin();

    auto c = ChatPtr::create();
    c->name = name;
    c->datetime = QDateTime::currentDateTime();

    auto db = QSqlDatabase::database(mDbConnection);
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO chats (name, datetime) VALUES (:name, :datetime)");
    q.bindValue(":name", c->name);
    q.bindValue(":datetime", c->datetime.toMSecsSinceEpoch());
    if (!q.exec())
    {
        qDebug() << q.lastError();
        return 0;
    }

    c->id = q.lastInsertId().toInt();

    beginInsertRows(QModelIndex(), 0, 0);
    mChats.prepend(c->id);
    mChatsHash[c->id] = c;
    endInsertRows();

    return c->id;
}

void ChatsModel::remove(qint32 chatId)
{
    dbBegin();

    auto db = QSqlDatabase::database(mDbConnection);
    QSqlQuery q(db);
    q.prepare("DELETE FROM chats WHERE id=:id");
    q.bindValue(":id", chatId);
    if (!q.exec())
    {
        qDebug() << q.lastError();
        return;
    }

    q.prepare("DELETE FROM messages WHERE chat_id=:id");
    q.bindValue(":id", chatId);
    if (!q.exec())
    {
        qDebug() << q.lastError();
        return;
    }

    const auto idx = mChats.indexOf(chatId);
    if (idx < 0)
        return;

    beginRemoveRows(QModelIndex(), idx, idx);
    mChatsHash.remove(chatId);
    mChats.removeAt(idx);
    endRemoveRows();
}

void ChatsModel::clear()
{
    dbBegin();

    auto db = QSqlDatabase::database(mDbConnection);
    QSqlQuery q(db);
    q.prepare("DELETE FROM chats");
    if (!q.exec())
    {
        qDebug() << q.lastError();
        return;
    }

    q.prepare("DELETE FROM messages");
    if (!q.exec())
    {
        qDebug() << q.lastError();
        return;
    }

    beginResetModel();
    mChatsHash.clear();
    mChats.clear();
    endResetModel();
}

bool ChatsModel::initDatabase()
{
    if (QSqlDatabase::contains(mDbConnection))
        return false;

    auto db = QSqlDatabase::addDatabase("QSQLITE", mDbConnection);
    db.setDatabaseName(mFileLocation);
    if (!db.open())
    {
        qDebug() << db.lastError();
        return false;
    }

    const auto version = dbGetValue("version", "0").toInt();
    QStringList queries;
    switch (version)
    {
    case 0:
        queries << R"(CREATE TABLE "chats" (
                      "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
                      "name" TEXT NOT NULL,
                      "datetime" INTEGER NOT NULL
                    ))";
        queries << R"(CREATE TABLE "general" (
                      "_key" TEXT NOT NULL,
                      "_value" TEXT NOT NULL,
                      PRIMARY KEY ("_key")
                    ))";
        queries << R"(CREATE TABLE "messages" (
                      "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
                      "chat_id" INTEGER NOT NULL,
                      "model" TEXT NOT NULL,
                      "role" TEXT NOT NULL,
                      "content" TEXT NOT NULL,
                      "datetime" INTEGER NOT NULL,
                      CONSTRAINT "messages_chat_id_frgkey" FOREIGN KEY ("chat_id") REFERENCES "chats" ("id") ON DELETE CASCADE ON UPDATE CASCADE
                    ))";
        break;
    }

    dbBegin();
    for (const auto &t: queries)
    {
        QSqlQuery q(db);
        q.prepare(t);
        q.exec();
    }

    if (queries.size())
        dbSetValue("version", "1");

    return true;
}

void ChatsModel::destroyDatabase()
{
    if (mDbConnection.isEmpty())
        return;

    dbCommit();
    QSqlDatabase::database(mDbConnection).close();
    QSqlDatabase::removeDatabase(mDbConnection);
}

QString ChatsModel::dbGetValue(const QString &key, const QString &defaultValue)
{
    auto db = QSqlDatabase::database(mDbConnection);
    QSqlQuery q(db);
    q.prepare("SELECT _value FROM general WHERE _key = :key");
    q.bindValue(":key", key);
    if (!q.exec())
    {
        qDebug() << q.lastError();
        return defaultValue;
    }

    if (!q.next())
        return defaultValue;

    return q.record().value(0).toString();
}

bool ChatsModel::dbSetValue(const QString &key, const QString &value)
{
    dbBegin();

    auto db = QSqlDatabase::database(mDbConnection);
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO general (_key, _value) VALUES (:key, :value)");
    q.bindValue(":key", key);
    q.bindValue(":value", value);
    if (!q.exec())
    {
        qDebug() << q.lastError();
        return false;
    }
    return true;
}
