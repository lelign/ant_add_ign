#include <sys/sysinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE >= 199309L
#endif
#define _XOPEN_SOURCE 700
#include <time.h>
#include "utils.h"

const char * timezone_format = "ln -sf /usr/share/zoneinfo/%s /etc/localtime";
const char * net_link_file = "/sys/class/net/eth0/operstate";
time_t time_monotone()
{
        struct timespec spec;
        clock_gettime(CLOCK_MONOTONIC, &spec);

        return spec.tv_sec;
}

unsigned int system_uptime(void)
{
        struct sysinfo info;
        sysinfo(&info);
        return info.uptime;        
}

void update_timezone(const char * timezone)
{
        char buf[256];

        snprintf(buf, sizeof(buf), timezone_format, timezone);
        system(buf);

}

int system_link_up(void)
{
        FILE * f;
        char state[65];
        int ret;

        memset(state, 0, sizeof(state));
        f = fopen(net_link_file, "r");
        if(!f)
                return LINK_DOWN;
        fscanf(f, "%64s", state);
        fclose(f);
        if(strcmp(state, "down")!=0)
                ret = LINK_UP;
        else
                ret = LINK_DOWN;
        return ret;
}

/**
* Перенастраивает IP адреса
**/
void ip_change()
{
        system("ifconfig eth0 down");
        sleep(1);
        system("/etc/init.d/networking restart");
}
