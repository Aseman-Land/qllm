#ifndef MODELMANAGER_H
#define MODELMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class ModelManagerItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged FINAL)
    Q_PROPERTY(QString name READ name CONSTANT FINAL)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(qint64 downloaded READ downloaded NOTIFY downloadedChanged FINAL)
    Q_PROPERTY(qint64 total READ total NOTIFY totalChanged FINAL)
    Q_PROPERTY(bool downloading READ downloading NOTIFY downloadingChanged FINAL)

public:
    ModelManagerItem(const QString &name, QObject *parent = nullptr);
    virtual ~ModelManagerItem();

    QString baseUrl() const;
    void setBaseUrl(const QString &newBaseUrl);

    QString name() const;

    QString status() const;
    qint64 downloaded() const;
    qint64 total() const;
    bool downloading() const;

Q_SIGNALS:
    void baseUrlChanged();
    void statusChanged();
    void downloadedChanged();
    void totalChanged();
    void downloadingChanged();
    void cancelRequest();

public Q_SLOTS:
    void start();
    void stop();

protected:
    void setTotal(qint64 newTotal);
    void setStatus(const QString &newStatus);
    void setDownloaded(qint64 newDownloaded);

private:
    qint64 mTotal = 0;
    qint64 mDownloaded = 0;
    QString mStatus;
    QString mName;

    QString mBaseUrl;

    QNetworkReply *mActiveReply = nullptr;
    QNetworkAccessManager *mAm;
};


class ModelManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged FINAL)
    Q_PROPERTY(QList<ModelManagerItem *> items READ items NOTIFY itemsChanged FINAL)

public:
    ModelManager(QObject *parent = nullptr);
    virtual ~ModelManager();

    QString baseUrl() const;
    void setBaseUrl(const QString &newBaseUrl);

    QList<ModelManagerItem *> items() const;

    static ModelManager *instance();

Q_SIGNALS:
    void baseUrlChanged();
    void itemsChanged();

public Q_SLOTS:
    ModelManagerItem *pull(const QString &name);
    void remove(const QString &name);
    void reload();

private:
    QString mBaseUrl;
    QMap<QString, ModelManagerItem*> mItems;
    QNetworkAccessManager *mAm;
};

#endif // MODELMANAGER_H
