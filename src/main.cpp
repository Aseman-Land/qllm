#include "mainwindow.h"
#include "chatsession.h"
#include "chatsmodel.h"
#include "settingsdialog.h"
#include "modelmanager.h"

#include <QApplication>
#include <QStyleFactory>

#ifdef QT_QUICKCONTROLS2_LIB
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDateTime>
#include <QQuickStyle>
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("qllm");
    app.setApplicationDisplayName("QLLM");
    app.setApplicationVersion("0.9.5");
    app.setOrganizationName("Aseman");
    app.setWindowIcon(QIcon(":/ui/icons/icon.svg"));

#ifdef QT_QUICKCONTROLS2_LIB
    if (!app.arguments().contains("--widgets"))
    {
        QScopedPointer<ChatsModel> chatsModel(new ChatsModel);
        auto session = new ChatSession(chatsModel.get(), chatsModel.get());

        qmlRegisterSingletonInstance("QLLM", 1, 0, "ChatsModel", chatsModel.get());
        qmlRegisterSingletonInstance("QLLM", 1, 0, "ChatSession", session);
        qmlRegisterType<ModelManager>("QLLM", 1, 0, "ModelManager");

        const auto desktop = qgetenv("DESKTOP_SESSION");
        const auto stylesList = QQuickStyle::availableStyles();
        const QSet<QString> styles(stylesList.begin(), stylesList.end());
        if (styles.contains("org.kde.desktop") && desktop == "plasma")
            QQuickStyle::setStyle("org.kde.desktop");
        else if (styles.contains("Plasma") && desktop == "plasma")
            QQuickStyle::setStyle("Plasma");
        else if (styles.contains("Suru"))
            QQuickStyle::setStyle("Suru");
        else
            QQuickStyle::setStyle("Default");

        QQmlApplicationEngine engine;
        engine.load(QUrl("qrc:/qml/main.qml"));

        return app.exec();
    }
    else
#endif
    {
        // qDebug() << QStyleFactory::keys();
        // auto style = QStyleFactory::create("Adwaita-Dark");
        // if (style)
        //     app.setStyle(style);
        MainWindow win;
        win.show();

        return app.exec();
    }
}
