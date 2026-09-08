#ifndef I2C_H
#define I2C_H

#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

class I2c
{
public:
    I2c(const char *i2c_filename);
    int read(unsigned char sAddr, unsigned char rAddr);
    int write(unsigned char sAddr, int rAddr, unsigned char value);

private:
    const char *i2c_path;
    int i2c_smbus_access(int file, char read_write, __u8 command,
                         int size, union i2c_smbus_data *data);
    int i2c_smbus_read_byte_data(int file, unsigned char reg);
    int i2c_ini(int addr);
};

#endif // I2C_H
