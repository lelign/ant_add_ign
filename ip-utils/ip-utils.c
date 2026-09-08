#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define INTERFACES_FILE "/etc/network/interfaces"
static const char * target_iface="eth0";
static const char * iface_template = "iface %s inet static";
static const char * ip_template = "\taddress %d.%d.%d.%d";
const char * mask_template = "\tnetmask %d.%d.%d.%d";
const char * gw_template = "\tgateway %d.%d.%d.%d";

const char * config_template = "auto lo\n\
iface lo inet loopback\n\
\n\
auto %s\n\
iface %s inet static\n\
\taddress %d.%d.%d.%d\n\
\tnetmask %d.%d.%d.%d\n\
\tgateway %d.%d.%d.%d\n\
\n";

int ip_get_ip(const char *name, char * ip, char * mask, char * gw)
{
        FILE * f;
        char iface[128+1];
        char buffer[128+1];
        unsigned int i;
        int ret;
        int r = 1;
        int ip1, ip2, ip3, ip4;
        int mask1, mask2, mask3, mask4;
        int gw1, gw2, gw3, gw4;

        bzero(iface, sizeof(iface));
        f = fopen(INTERFACES_FILE, "r");
        if(!f)
                return 1;
        while(!feof(f)){
                for(i=0; i<sizeof(buffer); i++){
                        ret = fread(buffer+i, 1, 1, f);
                        if(!ret)
                                break;
                        if(buffer[i]=='\n')
                                break;
                }
                buffer[i]='\0';
                ret=sscanf(buffer, iface_template, iface);
                if(strcmp(name, iface)==0){
                        if(sscanf(buffer, ip_template, &ip1, &ip2, &ip3, &ip4)==4){
                                sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
                                r = 0;
                        }
                        if(sscanf(buffer, mask_template, &mask1, &mask2, &mask3, &mask4)==4){
                                sprintf(mask, "%d.%d.%d.%d", mask1, mask2, mask3, mask4);
                                r = 0;
                        }
                        if(sscanf(buffer, gw_template, &gw1, &gw2, &gw3, &gw4)==4){
                                sprintf(gw, "%d.%d.%d.%d", gw1, gw2, gw3, gw4);
                                r = 0;
                        }
                }
        }
        fclose(f);
        return r;
}

void write_config(unsigned char * ip, unsigned char * netmask, unsigned char * gateway)
{
        FILE * f;

        f = fopen(INTERFACES_FILE, "w+");

        fprintf(f, config_template, \
        target_iface, \
        target_iface, \
        ip[0], ip[1], ip[2], ip[3], \
        netmask[0], netmask[1], netmask[2], netmask[3], \
        gateway[0], gateway[1], gateway[2], gateway[3] \
        );
        fclose(f);

        // Применяем изменения
        system("ifconfig eth0 down");
        sleep(1);
        system("/etc/init.d/networking restart");
}
