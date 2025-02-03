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

#define COLOR_TO_RGBA_STR(COLOR, ALPHA) QStringLiteral("rgba(%1, %2, %3, %4)").arg(COLOR.red()).arg(COLOR.green()).arg(COLOR.blue()).arg(ALPHA)

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
    ui->conversations->setModel(mChatsModel);
    ui->prompt->installEventFilter(this);

    const auto model = mSettings->value("Ollama/model").toString();

    mModelsCombo = new ModelsComboBox;
    mModelsCombo->setInitialModel(model);

    ui->secondSideModel->setInitialModel(model);

    auto settingsBtn = new QToolButton();
    settingsBtn->setAutoRaise(true);
    settingsBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    settingsBtn->setDefaultAction(ui->actionSettings);
    settingsBtn->setIconSize(QSize(24,24));

    auto modelsBtn = new QToolButton();
    modelsBtn->setAutoRaise(true);
    modelsBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    modelsBtn->setDefaultAction(ui->actionManage_Models);
    modelsBtn->setIconSize(QSize(24,24));

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

    initStyles();
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
    ui->conversations->setCurrentIndex(mChatsModel->indexOf(mSession->currentChat()));
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

void MainWindow::on_conversations_clicked(const QModelIndex &index)
{
    mSession->setCurrentChat( mChatsModel->chatId(index) );
}

void MainWindow::on_conversations_customContextMenuRequested(const QPoint &)
{
    const auto chatId = mChatsModel->chatId(ui->conversations->currentIndex());
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

QString MainWindow::readStyle(const QString &file) const
{
    QFile f(file);
    f.open(QFile::ReadOnly);
    auto styles = QString::fromUtf8(f.readAll());
    f.close();

    return styles;
}

void MainWindow::initStyles()
{
    const auto desktop = qgetenv("DESKTOP_SESSION");

    QStringList files = {
        ":/ui/stylesheets/chats.css",
    };

    const auto isPlasma = (desktop == "plasma");
    if (!isPlasma)
    {
        files += ":/ui/stylesheets/main.css";

        ui->headerFrame->setFrameShape(QFrame::Shape::NoFrame);
        ui->headerFrame->setFrameShadow(QFrame::Shadow::Plain);
        ui->headerFrame->setLineWidth(0);

        ui->chatsFrame->setFrameShape(QFrame::Shape::NoFrame);
        ui->chatsFrame->setFrameShadow(QFrame::Shadow::Plain);
        ui->chatsFrame->setLineWidth(0);

        ui->sendFrame->setFrameShape(QFrame::Shape::NoFrame);
        ui->sendFrame->setFrameShadow(QFrame::Shadow::Plain);
        ui->sendFrame->setLineWidth(0);

        ui->conversationsDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
        ui->conversationsDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    }

    const auto plt = palette();
    const auto textColor = plt.text().color();
    const auto isDark = (textColor.redF() + textColor.greenF() + textColor.blueF()) / 3 > 0.5;
    const auto areaColor = isDark? plt.base().color() : plt.window().color();
    const auto baseColor = isDark? plt.window().color() : plt.base().color();
    const auto highlightColor = plt.highlight().color();
    const auto borderAlpha = isDark? 0.1 : 0.3;

    if (isDark)
    {
        ui->actionSettings->setIcon(QIcon(":/ui/icons/configure-dark.svg"));
        ui->actionManage_Models->setIcon(QIcon(":/ui/icons/configure-dark.svg"));
        ui->actionNew_Conversation->setIcon(QIcon(":/ui/icons/list-add-dark.svg"));
    }
    else
    {
        ui->actionSettings->setIcon(QIcon(":/ui/icons/configure.svg"));
        ui->actionManage_Models->setIcon(QIcon(":/ui/icons/configure.svg"));
        ui->actionNew_Conversation->setIcon(QIcon(":/ui/icons/list-add.svg"));
    }

    QString styles;
    for (const auto &f: files)
        styles += readStyle(f) + '\n';

    styles.replace("color(base)", isPlasma? areaColor.name() : baseColor.name());
    styles.replace("color(area)", isPlasma? baseColor.name() : areaColor.name());
    styles.replace("color(border)", COLOR_TO_RGBA_STR(textColor, borderAlpha));
    styles.replace("color(highlight)", highlightColor.name());
    styles.replace("color(hover)", COLOR_TO_RGBA_STR(highlightColor, 0.2));

    setStyleSheet(styles);
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

