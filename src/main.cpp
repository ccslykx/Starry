#include <QApplication>
#include "STray.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    STray *sTray = STray::instance(&a);
    sTray->show();

    return a.exec();
}
