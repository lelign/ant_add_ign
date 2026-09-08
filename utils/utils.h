#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

enum {
        LINK_UP, 
        LINK_DOWN, 
        LINK_SIZE
};

time_t time_monotone();
unsigned int system_uptime(void);
void update_timezone(const char * timezone);
int system_link_up(void);
void ip_change();

#ifdef __cplusplus
}
#endif

#endif
