#include "modelscombobox.h"

#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>


class ModelsComboBoxDelegate: public QAbstractItemDelegate
{
public:
    ModelsComboBoxDelegate(ModelsComboBox *parent = nullptr)
        : QAbstractItemDelegate(parent)
        , mCombo(parent)
    {
    }
    virtual ~ModelsComboBoxDelegate(){}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        const auto size = sizeHint(option, index);

        painter->setFont(mCombo->font());
        painter->drawText(QRect(QPoint(0,size.height()*index.row()),size), mCombo->itemText(index.row()));
    }
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        return QSize(200, 60);
    }

private:
    ModelsComboBox *mCombo;
};

ModelsComboBox::ModelsComboBox(QWidget *parent)
    : QComboBox{parent}
{
    mAm = new QNetworkAccessManager(this);
    // setItemDelegate(new ModelsComboBoxDelegate(this));

    setMinimumWidth(180);
}

ModelsComboBox::~ModelsComboBox()
{
}

QString ModelsComboBox::baseUrl() const
{
    return mBaseUrl;
}

void ModelsComboBox::setBaseUrl(const QString &newBaseUrl)
{
    if (mBaseUrl == newBaseUrl)
        return;
    mBaseUrl = newBaseUrl;
    reload();
    Q_EMIT baseUrlChanged();
}

QString ModelsComboBox::currentModel()
{
    return currentData().toMap().value("model").toString();
}

void ModelsComboBox::reload()
{
    QUrl url(mBaseUrl + "/tags");

    QNetworkRequest req;
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setUrl(url);

    auto reply = mAm->get(req);

    clear();

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

        for (const auto &item: models.toArray())
        {
            if (!item.isObject())
                continue;

            const auto obj = item.toObject();
            const auto name = obj.value("name").toString();
            addItem(name, obj.toVariantMap());
            if (name == mInitialModel)
            {
                mInitialModel.clear();
                setCurrentIndex(count()-1);
            }
        }
    });
}

QString ModelsComboBox::initialModel() const
{
    return mInitialModel;
}

void ModelsComboBox::setInitialModel(const QString &newInitialModel)
{
    mInitialModel = newInitialModel;
}
