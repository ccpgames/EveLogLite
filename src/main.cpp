#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("CCP");
    a.setOrganizationDomain("ccpgames.com");
    a.setApplicationName("LogLite");
    a.setApplicationVersion(APP_VERSION);
    MainWindow w("");
    w.show();

    return a.exec();
}
