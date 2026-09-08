#include <QLoggingCategory>

#include "hdmi_adv7513.h"
#include "hdmi_adv7513_config_map.h"

#if (BOARD_REV==0)
static const char * i2c_filename = "/dev/i2c-0";
#else
static const char * i2c_filename = "/dev/i2c-1";
#endif
#define ADV7513_I2C_ADDR 0x39
static QLoggingCategory category("ADV7513 HDMI Transmitter");

Hdmi_adv7513::Hdmi_adv7513()
{
    qCDebug(category) << "initialization";

    adv7513_i2c = new I2c(i2c_filename);

    timer_HPD_int_test = new QTimer;
    connect(timer_HPD_int_test, SIGNAL(timeout()), this, SLOT(slot_timer_ctrl_HPD_int()));
    timer_HPD_int_test->setInterval(1000);
    timer_HPD_int_test->start();

    hdmi_format = HDMI_HD;
    color_format = 1;
    adv_7513_set_config();
}

Hdmi_adv7513::~Hdmi_adv7513()
{
    delete adv7513_i2c;
    delete timer_HPD_int_test;
}


void Hdmi_adv7513::slot_timer_ctrl_HPD_int()
{
/*
 *  Контроль подключения кабеля HDMI
*/
#define INT_BIT 0x80

    int dat = adv7513_i2c->read(ADV7513_I2C_ADDR, 0x96);
    if((dat & INT_BIT) == 0 ) return;

    qCDebug(category) << "HPD Interrupt (Hot Plug Detection)"; 

    adv7513_i2c->write(ADV7513_I2C_ADDR, 0x96, INT_BIT); // clear interrupt bits
    adv_7513_set_config();
}


void Hdmi_adv7513::adv_7513_set_hdmi_format(hdmi_format_t format)
{
    hdmi_format= format;
    adv_7513_set_config();
}


void Hdmi_adv7513::adv_7513_set_config()
{
QList<reg_value_t> hdmi_map;

    switch(hdmi_format){
        case HDMI_SD : hdmi_map = hdmi_sd; break;
        case HDMI_HD : hdmi_map = hdmi_hd; break;
        default      : hdmi_map = hdmi_hd; break;
        }

    for(int i = 0; i < hdmi_map.size(); ++i){
        adv7513_i2c->write(ADV7513_I2C_ADDR, hdmi_map[i].addr, hdmi_map[i].value);
    }
    if(color_format){
        for(int i = 0; i < hdmi_color_rgb.size(); ++i){
            adv7513_i2c->write(ADV7513_I2C_ADDR, hdmi_color_rgb[i].addr, hdmi_color_rgb[i].value);
        }
    }
}

void Hdmi_adv7513::adv_7513_set_color(int rgb)
{
    color_format = rgb;
}
