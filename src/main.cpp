#include <QApplication>
#include "STray.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyleSheet(QStringLiteral(
        "QToolTip {"
        "  color: #FFFFFF;"
        "  background-color: rgba(28, 25, 23, 218);"
        "  border: 1px solid rgba(251, 146, 60, 180);"
        "  border-radius: 8px;"
        "  padding: 7px 10px;"
        "  font-size: 14px;"
        "}"));
    a.setQuitOnLastWindowClosed(false);
    STray *sTray = STray::instance(&a);
    sTray->show();

    QObject::connect(sTray, &STray::exiting, &a, &QApplication::quit);

    return a.exec();
}
