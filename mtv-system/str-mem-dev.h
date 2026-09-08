#ifndef STR_MEM_DEV_H
#define STR_MEM_DEV_H

#define STRMEM_IOCTL_BASE 12345

#define STRMEM_IOCTL_REG (STRMEM_IOCTL_BASE+1)

typedef struct {
        uint32_t block;
        uint32_t address;
        uint32_t data;
        uint32_t rw;
} strmem_reg_data;

enum {STR_REG_READ, STR_REG_WRITE};

#endif