#include <QDateTime>
#include <QDebug>
#include "qpps.h"

void QPps::arm_timer()
{
        QDateTime now = QDateTime::currentDateTime();
        timer.start(1000-now.time().msec());
}

void QPps::timeout()
{
        emit pps();
        arm_timer();
}

QPps::QPps()
{
    timer.setTimerType(Qt::PreciseTimer);
        timer.setSingleShot(true);
        connect(&timer, SIGNAL(timeout()), this, SLOT(timeout()));
        arm_timer();
}

QPps::~QPps()
{

}
