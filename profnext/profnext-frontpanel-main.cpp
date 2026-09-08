#include <QtCore/QCoreApplication>
#include <signal.h>
#include <QCommandLineParser>
#include <QDebug>
#include <QObject>
#include "profnext-test.h"

QT_USE_NAMESPACE

void app_exit(int)
{
        QCoreApplication::quit();
}

int main(int argc, char *argv[])
{
        QCoreApplication a(argc, argv);
        signal(SIGINT, app_exit);
        signal(SIGPIPE, SIG_IGN);

        ProfnextTest panel;

        qDebug() << "Front panel test";
        return a.exec();
}
