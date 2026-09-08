#ifndef HDMI_GS12170_H
#define HDMI_GS12170_H

#include <QObject>
#include <QDebug>
#include <QTimer>
#include <stdio.h>
#include "spi/spi.h"

#define ADR_PLUS_EMEM		0x2000
#define GS12170_SPI_TRANSACTION_BYTES   6

#define GS12170_ISP_REG		        0x0005
#define GS12170_ISP0_POS	        12
#define GS12170_PMA_TX_REG58	    0x203A
#define GS12170_SDI_AUDIO_EN_REG	0x0094
#define GS12170_INPUT_LOCK_REG		0x0003
#define GS12170_DATA_RATE_REP_REG	0x0007
#define GS12170_INPUT0_LOCK_MASK	0x0001
#define GS12170_INPUT0_RATE_MASK	0x0003

static const char *spi_0_filename = "/dev/spidev0.0";
static const char *spi_1_filename = "/dev/spidev0.1";

struct reg_value_t{
    int addr;
    int value;
};

class Hdmi_gs12170 : public QObject{
    Q_OBJECT
public:
    Hdmi_gs12170(const char *spi_path);
    ~Hdmi_gs12170();

signals:
    void gs12170_lock_rate_changed(int gs12170_rate);

private slots:
    void slot_update_gs12170();

private:
    void init_gs12170();
    void gs12170_write(uint16_t addr, uint16_t data);
    void gs12170_read(uint16_t addr, uint16_t *data);

    Spi *gs12170_spi;
    QTimer *timer_update_gs12170;

    int lock;
	int lock_prev;
    int rate;
    int rate_prev;
};

#endif // HDMI_GS12170_H