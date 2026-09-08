#ifndef TIME_COUNTER_H
#define TIME_COUNTER_H

#include <QLoggingCategory>
#include <QObject>
#include <QTimer>

class Time_counter : public QObject
{
    Q_OBJECT
public:
    explicit Time_counter(QObject *parent = nullptr);

signals:
    void signal_time_counter_update(QString time_count_str);

public slots:
    void slot_timeout();
    void slot_start();
    void slot_stop();

private:
    QTimer  timer_time_couter;
    qint64  time_count;

    QString timeConversion(qint64 secs);
    void    emit_signal_time_counter_update();
};

#endif // TIME_COUNTER_H
