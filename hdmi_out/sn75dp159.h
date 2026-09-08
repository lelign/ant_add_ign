#ifndef HDMI_SN75DP159_H
#define HDMI_SN75DP159_H

#include <QObject>
#include <QDebug>
#include <QTimer>
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <unistd.h>
#include "../i2c/i2c.h"

#define SN175DP_ADDR_0  0x5E
#define SN175DP_ADDR_1  0x5D

enum gs12170_rate_enum{
	RATE_HD_12170 = 0,
	RATE_3G_12170,
	RATE_6G_12170,
	RATE_12G_12170
};

static const char * i2c_filename_sn75dp = "/dev/i2c-0";

class Sn75dp159 : public QObject{
    Q_OBJECT
private:
    I2c *sn75dp_i2c;
    uint8_t sn75dp_addr;
    QTimer *timer_update_sn75dp;

    void sn75dp159_write(uint8_t reg_addr, uint8_t reg_data);
    void sn75dp159_read(uint8_t reg_addr, uint8_t *reg_data);
    bool sn75_Mode;// Флаг активации расширенных логов из аргументов командной строки

public slots:
    void sn75dp159_device_id_read();
    void slot_sn75dp159_update(int rate);

public:
    Sn75dp159(uint8_t i2c_addr);
    ~Sn75dp159();
};

#endif //HDMI_SN75DP159_H