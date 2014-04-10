#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName("ChatHack Group");
    QApplication::setApplicationName("BatChat");
    MainWindow w;
    w.show();

    return a.exec();
}
