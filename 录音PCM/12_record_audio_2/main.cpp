#include "mainwindow.h"

#include <QApplication>

extern "C" {
#include <libavdevice/avdevice.h>
}

int main(int argc, char *argv[])
{
    qputenv("QT_SCALE_FACTOR", QByteArray("1"));

    avdevice_register_all(); //注册设备

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
