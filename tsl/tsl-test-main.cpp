#include <QApplication>
#include <signal.h>
#include <QDebug>
#include <QObject>
#include "tsl-test.h"

QT_USE_NAMESPACE

void app_exit(int)
{
        QCoreApplication::quit();
}

int main(int argc, char *argv[])
{
        QApplication a(argc, argv);
        qDebug() << "PBX-MTV-508 TSL application";

        signal(SIGINT, app_exit);

        TslTest tsltest;

        return a.exec();
}
