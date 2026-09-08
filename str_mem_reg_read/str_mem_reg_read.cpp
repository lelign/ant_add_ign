#include "str_mem_reg_read.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>

// Отрезанный заголовок вашей структуры для ioctl (если он не в другом месте)
// #include "str-mem-dev.h" 

StrMemRegRead::StrMemRegRead()
{
    // Оставляем пустым или инициализируем внутренние переменные
}

uint32_t StrMemRegRead::reg_read(uint32_t block, uint32_t addr)
{
    strmem_reg_data reg_data;
    int ret;
    int fd;

    reg_data.block = block;
    reg_data.address = addr * 4;
    reg_data.data = 0;
    reg_data.rw = STR_REG_READ;

    fd = open("/dev/str-mem", O_RDONLY);
    if(fd < 0){
        return 0;
    }
    ret = ioctl(fd, STRMEM_IOCTL_REG, &reg_data);
    if(ret < 0){
        printf("ioctl error\n");
    }
    close(fd);
    
    return reg_data.data;
}
