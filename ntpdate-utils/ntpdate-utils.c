#include <stdio.h>

#include "ntpdate-utils.h"

static const char *ntpdate_conf_file = "/etc/default/ntpdate";
static const char *ntp_server_format = "NTPSERVERS=\"%s\"\n";
static const char *update_hwclock = "UPDATE_HWCLOCK=\"yes\"\n";

void ntpdate_server(const char * ntp_server)
{
        FILE * f;

        f = fopen(ntpdate_conf_file, "w+");
        if(!f)
                return;
        fprintf(f, ntp_server_format, ntp_server);
        //fprintf(f, update_hwclock);
        fprintf(f, "%s", update_hwclock); // ign
        fclose(f);
}
