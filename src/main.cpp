#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);
    a.setOrganizationName("CCP");
    a.setOrganizationDomain("ccpgames.com");
    a.setApplicationName("LogLite");
    a.setApplicationVersion(APP_VERSION);
    MainWindow w(a.arguments().value(1));
    w.show();

    return a.exec();
}
