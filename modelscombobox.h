#ifndef MODELSCOMBOBOX_H
#define MODELSCOMBOBOX_H

#include <QComboBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class ModelsComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged FINAL)
public:
    ModelsComboBox(QWidget *parent = nullptr);
    virtual ~ModelsComboBox();

    QString baseUrl() const;
    void setBaseUrl(const QString &newBaseUrl);

    QString currentModel();

    QString initialModel() const;
    void setInitialModel(const QString &newInitialModel);

Q_SIGNALS:
    void baseUrlChanged();

public Q_SLOTS:
    void reload();

private:
    QString mBaseUrl;
    QString mInitialModel;
    QNetworkAccessManager *mAm;
};

#endif // MODELSCOMBOBOX_H
