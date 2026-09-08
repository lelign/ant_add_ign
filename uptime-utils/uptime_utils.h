#ifndef UPTIME_UTILS_H
#define UPTIME_UTILS_H

#include <QObject>
#include <QTimer>
#include <sys/sysinfo.h>


class Uptime_utils : public QObject
{
    Q_OBJECT
public:
    Uptime_utils();
    QString  get_sys_uptime();

signals:

public slots:

private:

};

#endif // UPTIME_UTILS_H
