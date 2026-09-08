#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <QDebug>
#include <QTimer>
#include "watchdog.h"

const char * WATCHDOG_FILE = "/dev/watchdog";

Watchdog::Watchdog(int enable):
        watchdog_fh(-1)
{
        if(!enable)
                return;
        watchdog_fh = open(WATCHDOG_FILE, O_WRONLY);
        if(watchdog_fh<0)
                return;
        connect(&timer, &QTimer::timeout, this, &Watchdog::watchdog_kick);
        timer.start(1000);
        qDebug() << "Watchdog enabled";
}

Watchdog::~Watchdog()
{
        stop();
}
/**
* Корректное закрытие watchdog
**/
void Watchdog::stop()
{
        char c[1]={'V'};
        if(watchdog_fh<0)
                return;
        write(watchdog_fh, c, 1);
        close(watchdog_fh);
}

/**
* Необходимо переодически вызывать эту функцию, иначе перезагрузка через 16с
**/
void Watchdog::watchdog_kick()
{
        char c[1]={0};
        if(watchdog_fh<0)
                return;
        //qDebug() << "watchdog_kick()";
        write(watchdog_fh, c, 1);
}
