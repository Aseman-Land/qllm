#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QSharedPointer>
#include <QSettings>
#include <QTimer>

#include "chatsmodel.h"
#include "chatsession.h"
#include "modelscombobox.h"
#include "settingsdialog.h"
#include "messageitem.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public Q_SLOTS:
    void send();

private Q_SLOTS:
    void on_sendBtn_clicked();
    void on_listView_clicked(const QModelIndex &index);
    void on_listView_customContextMenuRequested(const QPoint &pos);
    void on_actionNew_Conversation_triggered();
    void on_actionSettings_triggered();
    void on_clearBtn_clicked();
    void on_actionManage_Models_triggered();
    void on_secondSideCheck_clicked();
    void on_secondSideModel_currentIndexChanged(int index);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *e) override;

    void print();
    void initSettings();
    void reloadPromptPlaceholder();
    void initAutoAnswer();
    void initBaseUrl();
    void initStyles();

    QString baseUrl() const;

private:
    Ui::MainWindow *ui;
    QSettings *mSettings;
    QTimer *mScrollTimer;

    ChatsModel *mChatsModel;
    ChatSession *mSession;

    ModelsComboBox *mModelsCombo = nullptr;
    SettingsDialog *mSettingsDialog = nullptr;

    QHash<ChatSession::Message*, MessageItem*> mMessages;
};
#endif // MAINWINDOW_H
