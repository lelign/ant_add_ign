#ifndef HDMI_ADV7513_H
#define HDMI_ADV7513_H

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


class Hdmi_adv7513 : public QObject
{
    Q_OBJECT
public:
    Hdmi_adv7513();
    ~Hdmi_adv7513();

    enum hdmi_format_t{ HDMI_SD, HDMI_HD};

    void adv_7513_set_hdmi_format(hdmi_format_t format);
    void adv_7513_set_color(int rgb);

private slots:
    void slot_timer_ctrl_HPD_int();

private:
//    int adv7513_I2C_ini(int addr);

//    int adv7513_I2C_read(unsigned char sAddr, unsigned char rAddr);
//    int adv7513_I2C_write(unsigned char sAddr, int cmd, unsigned char value);

//    int i2c_smbus_access(int file, char read_write, __u8 command,int size, union i2c_smbus_data *data);
//    int i2c_smbus_read_byte_data(int file, quint8 command);


    void adv_7513_set_config();
    hdmi_format_t hdmi_format;

    QTimer *timer_HPD_int_test;

    I2c *adv7513_i2c;
    int color_format;

};

#endif // HDMI_ADV7513_H
