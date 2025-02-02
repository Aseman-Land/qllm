#ifndef MESSAGEITEM_H
#define MESSAGEITEM_H

#include "chatsession.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class MessageItem;
}
QT_END_NAMESPACE

class MessageItem : public QWidget
{
    Q_OBJECT
public:
    explicit MessageItem(ChatSession::MessagePtr msg, ChatSession *session, QWidget *parent = nullptr);
    virtual ~MessageItem();

    ChatSession::MessagePtr message() const;

public Q_SLOTS:
    void refresh();

protected:
    static Qt::LayoutDirection directionOf(const QString &str);

private slots:
    void on_menuBtn_clicked();

private:
    Ui::MessageItem *ui;
    ChatSession::MessagePtr mMessage;
    ChatSession *mSession;
};

#endif // MESSAGEITEM_H
