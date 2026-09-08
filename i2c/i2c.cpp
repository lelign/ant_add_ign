#include "i2c.h"


I2c::I2c(const char * i2c_filename)
{
    i2c_path = i2c_filename;
}

int I2c::i2c_ini(int addr){
int file;

    //  Open the i2c bus
    if((file = open(i2c_path, O_RDWR)) < 0){
        printf("Failed to open the i2c bus\n");
        return -1;
    }

    if(ioctl(file, I2C_SLAVE, addr) < 0){
        printf("Failed to acquire bus access and/or talk to slave.\n");
        close(file);
        return -1;
    }

    return file;
}


int I2c::read(unsigned char sAddr, unsigned char rAddr){
int file_i2c;

    file_i2c = i2c_ini(sAddr);
    if(file_i2c < 0) return -1;

    int dat = i2c_smbus_read_byte_data(file_i2c, rAddr);

    close(file_i2c);
    return dat;
}


int I2c::write(unsigned char sAddr, int rAddr, unsigned char value)
{
int file_i2c;
int ret;
union i2c_smbus_data data;

    data.byte = value;
    file_i2c = i2c_ini(sAddr);
    if(file_i2c < 0) return -1;

    if(i2c_smbus_access(file_i2c,I2C_SMBUS_WRITE,rAddr, I2C_SMBUS_BYTE_DATA,&data))
        ret = -1;
    else
        ret = 0x0FF & data.byte;

    close(file_i2c);
    return ret;
}


int I2c::i2c_smbus_access(int file, char read_write, __u8 command,
                              int size, union i2c_smbus_data *data)
{
    struct i2c_smbus_ioctl_data args;

    args.read_write = read_write;
    args.command = command;
    args.size = size;
    args.data = data;
    return ioctl(file,I2C_SMBUS,&args);
}


int I2c::i2c_smbus_read_byte_data(int file, unsigned char reg)
{
    union i2c_smbus_data data;
    if (i2c_smbus_access(file,I2C_SMBUS_READ,reg,
                         I2C_SMBUS_BYTE_DATA,&data))
        return -1;
    else
        return 0x0FF & data.byte;
}

