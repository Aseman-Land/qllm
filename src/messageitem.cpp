#include "messageitem.h"
#include "ui_messageitem.h"

#include <QClipboard>
#include <QMenu>
#include <QMessageBox>
#include <QMetaMethod>
#include <QRegularExpression>

MessageItem::MessageItem(ChatSession::MessagePtr msg, ChatSession *session, QWidget *parent)
    : QWidget{parent}
    , ui(new Ui::MessageItem)
    , mMessage(msg)
    , mSession(session)
{
    ui->setupUi(this);

    QColor color;
    if (mMessage->role == "user")
    {
        setContentsMargins(4,30,4,4);
        color = QColor(85, 170, 255);
        color.setAlphaF(0.1);
    }
    else
    {
        const auto plt = palette();
        color = plt.base().color();
        color.setAlphaF(1);

        setContentsMargins(4,4,4,4);
    }

    ui->contentArea->setStyleSheet(QString(R"(QFrame#contentArea {
                                                background-color: rgba(%1, %2, %3, %4);
                                                border-radius: 10px;
                                            })").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alphaF()));

    refresh();
}

MessageItem::~MessageItem()
{
}

ChatSession::MessagePtr MessageItem::message() const
{
    return mMessage;
}

void MessageItem::refresh()
{
    auto content = mMessage->content;
    const auto dir = directionOf(content);

    const QRegularExpression thinkRx("(.*)\\<\\/(?:think|response)\\>", QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpression thinkRx2("\\<think\\>(.*)", QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption);
    auto match = thinkRx.match(mMessage->content);
    if (!match.hasMatch())
        match = thinkRx2.match(mMessage->content);
    if (match.hasMatch())
    {
        auto think = match.captured(0);
        content.remove(think);
        think.remove("<think>").remove("</think>").remove("</response>");

        ui->thinkArea->setVisible(true);
        ui->think->setText(think.trimmed());

    }
    else
        ui->thinkArea->setVisible(false);

    content = content.trimmed();
    setLayoutDirection(dir);

    if (mMessage->role == "user")
    {
        ui->content->setTextFormat(Qt::PlainText);
        ui->avatar->setText(QString::fromUtf8("🙂"));
        ui->sender->setText(tr("You"));
    }
    else
    {
        ui->content->setTextFormat(Qt::MarkdownText);
        ui->avatar->setText(QString::fromUtf8("🤖"));
        ui->sender->setText(mMessage->model);
    }

    ui->content->setText(content);
    ui->datetime->setText(mMessage->datetime.toString("yyyy-MM-dd hh:mm:ss"));
}

Qt::LayoutDirection MessageItem::directionOf(const QString &str)
{
    Qt::LayoutDirection res = Qt::LeftToRight;
    if( str.isEmpty() )
        return res;

    int ltr = 0;
    int rtl = 0;

    for(const QChar &ch: str)
    {
        QChar::Direction dir = ch.direction();
        switch( static_cast<int>(dir) )
        {
        case QChar::DirL:
        case QChar::DirLRE:
        case QChar::DirLRO:
        case QChar::DirEN:
            ltr++;
            break;

        case QChar::DirR:
        case QChar::DirRLE:
        case QChar::DirRLO:
        case QChar::DirAL:
            rtl++;
            break;
        }
    }

    if( ltr >= rtl * 3 )
        res = Qt::LeftToRight;
    else
        res = Qt::RightToLeft;

    return res;
}

void MessageItem::on_menuBtn_clicked()
{
    QMenu menu;
    auto copyAction = menu.addAction("Copy");
    auto deleteAction = menu.addAction("Delete");
    auto res = menu.exec( ui->menuBtn->mapToGlobal(ui->menuBtn->rect().bottomLeft()) );

    if (res == copyAction)
    {
        QApplication::clipboard()->setText( ui->content->text() );
    }
    else if (res == deleteAction)
    {
        if (QMessageBox::warning(this, tr("Delete"), tr("Are you sure about delete this message?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
            return;

        QMetaObject::invokeMethod(this, [this](){
            mSession->deleteMessage(mMessage);
        }, Qt::QueuedConnection);
    }
}

