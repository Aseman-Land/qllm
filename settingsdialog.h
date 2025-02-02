#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <QSettings>

#include "modelmanager.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QSettings *settings, QWidget *parent = nullptr);
    ~SettingsDialog();

    ModelManager *modelManager() const;

    QString baseUrl() const;
    void setBaseUrl(const QString &newBaseUrl);

    void setCurrentTab(int index);

    void accept() override;

private Q_SLOTS:
    void on_listWidget_currentRowChanged(int currentRow);
    void on_pullModelBtn_clicked();

protected:
    void itemsChanged();

private:
    Ui::SettingsDialog *ui;
    QSettings *mSettings;
    QString mBaseUrl;

    ModelManager *mModelManager = nullptr;

    QHash<QString, QListWidgetItem*> mItems;
};

#endif // SETTINGSDIALOG_H
