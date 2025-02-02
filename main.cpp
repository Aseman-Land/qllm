#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("qllm");
    app.setApplicationDisplayName("QLLM");
    app.setApplicationVersion("0.9.5");
    app.setOrganizationName("Aseman");
    app.setWindowIcon(QIcon::fromTheme("applications-chat"));

    // auto style = QStyleFactory::create("Adwaita-Dark");
    // if (style)
    //     app.setStyle(style);

    MainWindow win;
    win.show();

    return app.exec();
}
