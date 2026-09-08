#include "time_counter.h"

static QLoggingCategory category("Time_counter");

Time_counter::Time_counter(QObject *parent) : QObject(parent)
{
    qCDebug(category) << "creating...";

    time_count = 0;
    timer_time_couter.setInterval(1000);
    connect(&timer_time_couter, &QTimer::timeout, this, &Time_counter::slot_timeout);
}

void Time_counter::slot_timeout()
{
    time_count++;
    if(time_count >= (100 * 60 * 60)) time_count = 0;

    emit_signal_time_counter_update();
}

QString Time_counter::timeConversion(qint64 secs)
{
    QString formattedTime;

    int hours   = (secs / 60 / 60);
    int minutes = (secs / 60) % 60;
    int seconds =  secs  % 60;

    formattedTime.append(QString("%1"  ).arg(hours,   2, 10, QLatin1Char('0')) + ":" +
                         QString( "%1" ).arg(minutes, 2, 10, QLatin1Char('0')) + ":" +
                         QString( "%1" ).arg(seconds, 2, 10, QLatin1Char('0')));

    return formattedTime;
}

void Time_counter::slot_start()
{
    timer_time_couter.start();
    emit_signal_time_counter_update();
}

void Time_counter::slot_stop()
{
    timer_time_couter.stop();
    time_count = 0;
    emit_signal_time_counter_update();
}

void Time_counter::emit_signal_time_counter_update()
{
    QString time_count_str = timeConversion(time_count);
    emit signal_time_counter_update(time_count_str);
}
