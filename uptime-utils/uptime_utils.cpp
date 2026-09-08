#include "uptime_utils.h"

Uptime_utils::Uptime_utils()
{
}

QString  Uptime_utils::get_sys_uptime()
/* Seconds since boot */
{
struct sysinfo sys_info;
QString uptime_str;
int days, hours, minutes;
quint32 uptime_sec;

    if(sysinfo(&sys_info)) return "No Information";
    uptime_sec =sys_info.uptime;

    days    =  uptime_sec / 60 / 60 / 24;
    hours   = (uptime_sec / 60 / 60) % 24;
    minutes = (uptime_sec / 60) % 60;

    if((hours == 0)&&(days == 0))
        uptime_str = QString("%1 minutes").arg(minutes);
    if((days == 0) && hours)
        uptime_str = QString("%1 hours, %2 minutes")
            .arg(hours)
            .arg(minutes);

    if(days)
        uptime_str = QString("%1 days, %2 hours").arg(days).arg(hours);

    return uptime_str;
}

