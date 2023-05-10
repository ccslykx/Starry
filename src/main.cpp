#include "tray.h"

int main(int argc, char *argv[]) 
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false); // 关闭最后一个窗口后，托盘也不退出

    PopupTray *tray = PopupTray::instance(&a);
    tray->show();
    
    return a.exec();
}