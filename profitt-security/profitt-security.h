#ifndef PROFITT_SECURITY_H
#define PROFITT_SECURITY_H

#define PASSWORD_MAX_LEN 64
#define PASSWORD_FILE "/etc/profitt_password"
#define PASSWORD_EMPTY ""
#define PROFITT_DI_FILE "/var/volatile/proflex-ip-disable"

enum {PROFITT_DI_DISABLE, PROFITT_DI_ENABLE};

void secutiry_set_password(const char * password);
void secutiry_set_profitt_di(int profitt_di);
#endif

