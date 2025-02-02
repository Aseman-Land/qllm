#ifndef CHATSMODEL_H
#define CHATSMODEL_H

#include <QAbstractItemModel>
#include <QDateTime>
#include <QSharedPointer>
#include <QTimer>

class ChatsModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(QString fileLocation READ fileLocation WRITE setFileLocation NOTIFY fileLocationChanged FINAL)

public:
    ChatsModel(QObject *parent = nullptr);
    virtual ~ChatsModel();

    int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    QModelIndex indexOf(qint32 chatId) const;
    qint32 chatId(QModelIndex index) const;

    QString fileLocation() const;
    void setFileLocation(const QString &newFileLocation);

    QString dbConnection() const;
    void dbBegin(bool autoCommit = true);
    void dbCommit();

public Q_SLOTS:
    void reload();
    qint32 create(const QString &name);
    void remove(qint32 chatId);
    void clear();

Q_SIGNALS:
    void fileLocationChanged();

protected:
    bool initDatabase();
    void destroyDatabase();

    QString dbGetValue(const QString &key, const QString &defaultValue = QString());
    bool dbSetValue(const QString &key, const QString &value);

private:
    QString mFileLocation;
    QString mDbConnection;

    struct Chat
    {
        qint32 id;
        QString name;
        QDateTime datetime;
    };
    typedef QSharedPointer<Chat> ChatPtr;

    QTimer *mDbCommitTimer = nullptr;

    QList<qint32> mChats;
    QHash<qint32, ChatPtr> mChatsHash;
};

#endif // CHATSMODEL_H
