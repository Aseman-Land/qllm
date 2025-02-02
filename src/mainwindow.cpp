#include "mainwindow.h"
#include "settingsdialog.h"
#include "./ui_mainwindow.h"

#include <QVariantMap>
#include <QScrollBar>
#include <QStandardPaths>
#include <QDir>
#include <QMenu>
#include <QMessageBox>
#include <QLabel>
#include <QToolButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    const auto dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir);

    mSettings = new QSettings(dataDir + "/settings.ini", QSettings::IniFormat, this);

    mChatsModel = new ChatsModel(this);
    mSession = new ChatSession(mChatsModel, this);

    mScrollTimer = new QTimer(this);
    mScrollTimer->setInterval(100);
    mScrollTimer->setSingleShot(true);

    connect(mScrollTimer, &QTimer::timeout, this, [this](){
        ui->messages->verticalScrollBar()->setValue(ui->messages->verticalScrollBar()->maximum());
    });

    connect(mSession, &ChatSession::messagesChanged, this, &MainWindow::print);

    ui->setupUi(this);
    ui->listView->setModel(mChatsModel);
    ui->prompt->installEventFilter(this);

    const auto model = mSettings->value("Ollama/model").toString();

    mModelsCombo = new ModelsComboBox;
    mModelsCombo->setInitialModel(model);

    ui->secondSideModel->setInitialModel(model);

    auto settingsBtn = new QToolButton();
    settingsBtn->setAutoRaise(true);
    settingsBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    settingsBtn->setDefaultAction(ui->actionSettings);

    auto modelsBtn = new QToolButton();
    modelsBtn->setAutoRaise(true);
    modelsBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    modelsBtn->setDefaultAction(ui->actionManage_Models);

    auto toolbarWidget = new QWidget;
    auto toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->addWidget(new QLabel("Model:"));
    toolbarLayout->addWidget(mModelsCombo);
    toolbarLayout->addWidget(modelsBtn);
    toolbarLayout->addStretch();
    toolbarLayout->setContentsMargins(10,0,0,0);
    toolbarLayout->addWidget(settingsBtn);

    ui->toolBar->addWidget(toolbarWidget);

    mChatsModel->setFileLocation(dataDir + "/conversations.sqlite");

    connect(mModelsCombo, static_cast<void(ModelsComboBox::*)(int)>(&ModelsComboBox::currentIndexChanged), this, &MainWindow::reloadPromptPlaceholder);

    restoreGeometry(mSettings->value("UI/geometry").toByteArray());
    // restoreState(mSettings->value("UI/docks").toByteArray());

    initBaseUrl();
    reloadPromptPlaceholder();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::send()
{
    const auto prompt = ui->prompt->toPlainText().trimmed();
    ui->prompt->clear();

    if (prompt.isEmpty())
        return;

    mSession->sendPrompt(mModelsCombo->currentModel(), prompt);
    ui->listView->setCurrentIndex(mChatsModel->indexOf(mSession->currentChat()));
}

void MainWindow::on_sendBtn_clicked()
{
    send();
}

void MainWindow::print()
{
    QString text;
    bool allowPrint = false;
    const auto messages = mSession->messages();
    QSet<ChatSession::Message*> addedMessages;
    QList<MessageItem*> items;
    for (const auto &msg: messages)
    {
        addedMessages.insert(msg.get());
        auto &item = mMessages[msg.get()];
        if (item)
        {
            items << item;
            continue;
        }

        item = new MessageItem(msg, mSession);
        ui->messagesLayout->insertWidget(ui->messagesLayout->count()-1, item);
        items << item;
    }

    const auto keys = mMessages.keys();
    for (const auto k: keys)
        if (!addedMessages.contains(k))
        {
            auto item = mMessages.take(k);
            delete item;
        }

    if (!items.isEmpty())
        items.last()->refresh();

    if (!mScrollTimer->isActive())
        mScrollTimer->start();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->prompt)
    {
        if (event->type() == QEvent::KeyPress)
        {
            auto keyEvent = static_cast<QKeyEvent *>(event);
            switch (keyEvent->key())
            {
            case Qt::Key_Return:
            case Qt::Key_Enter:
                if (keyEvent->modifiers() == Qt::NoModifier)
                {
                    send();
                    return true;
                }
                break;
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    mSettings->setValue("UI/geometry", saveGeometry());
    mSettings->setValue("UI/docks", saveState());
    e->accept();
}

void MainWindow::on_listView_clicked(const QModelIndex &index)
{
    mSession->setCurrentChat( mChatsModel->chatId(index) );
}

void MainWindow::on_listView_customContextMenuRequested(const QPoint &)
{
    const auto chatId = mChatsModel->chatId(ui->listView->currentIndex());
    if (!chatId)
        return;

    QMenu menu;
    auto openAct = menu.addAction(tr("Open"));
    menu.addSeparator();
    auto deleteAct = menu.addAction(tr("Delete"));

    auto res = menu.exec(QCursor::pos());

    if (res == openAct)
    {
        mSession->setCurrentChat(chatId);
    }
    else if (res == deleteAct)
    {
        if (QMessageBox::warning(this, tr("Delete"), tr("Are you sure about delete this conversation?"), QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes)
            return;

        mChatsModel->remove(chatId);
        mSession->setCurrentChat(0);
    }
}

void MainWindow::on_actionNew_Conversation_triggered()
{
    mSession->setCurrentChat(0);
}

void MainWindow::initSettings()
{
    if (mSettingsDialog)
        return;

    mSettingsDialog = new SettingsDialog(mSettings, this);
}

void MainWindow::reloadPromptPlaceholder()
{
    mSettings->setValue("Ollama/model", mModelsCombo->currentText());
    ui->prompt->setPlaceholderText(
            ui->secondSideCheck->isChecked()?
                tr("Say Something to %1 to start discussion...").arg(mModelsCombo->currentText()) :
                tr("Say Something to %1...").arg(mModelsCombo->currentText())
        );
}

void MainWindow::initAutoAnswer()
{
    if (ui->secondSideCheck->isChecked())
    {
        mSession->setAutoAnswerModel(ui->secondSideModel->currentModel());
    }
    else
        mSession->setAutoAnswerModel(QString());

    reloadPromptPlaceholder();
}

QString MainWindow::baseUrl() const
{
    const auto api = QStringLiteral("/api");

    auto host = mSettings->value("Ollama/host", "localhost").toString();
    if (host.left(7) != QStringLiteral("http://") && host.left(8) != QStringLiteral("https://"))
        host = QStringLiteral("http://") + host;

    const auto port = mSettings->value("Ollama/port", 11434).toInt();
    if (port)
        return host + ':' + QString::number(port) + api;
    else
        return host + api;
}

void MainWindow::initBaseUrl()
{
    mSession->setBaseUrl(baseUrl());
    mModelsCombo->setBaseUrl(baseUrl());
    ui->secondSideModel->setBaseUrl(baseUrl());
}

void MainWindow::on_actionSettings_triggered()
{
    initSettings();

    mSettingsDialog->setBaseUrl(baseUrl());
    mSettingsDialog->setCurrentTab(0);
    mSettingsDialog->exec();

    initBaseUrl();
}

void MainWindow::on_clearBtn_clicked()
{
    if (QMessageBox::warning(this, tr("Clear"), tr("Are you sure about clear all conversations?"), QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes)
        return;

    mChatsModel->clear();
    mSession->setCurrentChat(0);
}

void MainWindow::on_actionManage_Models_triggered()
{
    initSettings();
    mSettingsDialog->setCurrentTab(1);
    mSettingsDialog->exec();
}

void MainWindow::on_secondSideCheck_clicked()
{
    ui->secondSideModel->setDisabled(!ui->secondSideCheck->isChecked());
    initAutoAnswer();
}

void MainWindow::on_secondSideModel_currentIndexChanged(int)
{
    initAutoAnswer();
}

