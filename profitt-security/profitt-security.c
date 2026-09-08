#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "profitt-security.h"

/**
* Установить пароль
* Далее запускается программа security_update.sh из пакета profitt-security,
* которая устанавливает пароли в системе
**/
void secutiry_set_password(const char * password)
{
        FILE * f;
        f = fopen(PASSWORD_FILE, "w+");
        fwrite(password, 1, strlen(password), f);
        fclose(f);
        system("/etc/init.d/security_update.sh restart");
}

/**
* Включить-выключить установку адреса через profitt-di
* требуется программа proflex-ip версии 0.5 и выше
* @param profitt_di 1 включить
* @param profitt_di 0 выключить
**/
void secutiry_set_profitt_di(int profitt_di)
{
        FILE * f;

        if(profitt_di==PROFITT_DI_DISABLE){
                f = fopen(PROFITT_DI_FILE, "w+");
                fclose(f);
        }
        else{
                unlink(PROFITT_DI_FILE);
        }
}