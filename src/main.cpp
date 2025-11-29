#include <QApplication>
#include "STray.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    STray *sTray = STray::instance(&a);
    sTray->show();

    QObject::connect(sTray, &STray::exiting, &a, &QApplication::quit);

    return a.exec();
}
