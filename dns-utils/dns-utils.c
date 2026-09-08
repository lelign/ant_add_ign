#include <stdio.h>

#include "dns-utils.h"

static const char *resolv_conf_file = "/etc/resolv.conf";
static const char *resolv_conf_format = "nameserver %s\n";

void update_resolvconf(const char * dns)
{
        FILE * f;

        f = fopen(resolv_conf_file, "w+");
        if(!f)
                return;
        fprintf(f, resolv_conf_format, dns);
        fclose(f);
}
