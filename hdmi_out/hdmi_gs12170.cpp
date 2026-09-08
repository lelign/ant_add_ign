#include <QLoggingCategory>
#include "hdmi_gs12170.h"

static QLoggingCategory category("GS21170 SDI<->HDMI Bridge");

QList<reg_value_t> sdi_hdmi = {
    {0x007C, 0x3668},
    {0x00DC, 0x0002},
    {0x1065, 0x0007},
    {0x10F4, 0x0001},
    {0x10F8, 0x0000},
    {0x10FA, 0x0000},
    {0x10F9, 0x0078},
    {0x10F7, 0x0810},
    {0x10F5, 0x0001},
    {0x10F9, 0x0078},
    {0x10F7, 0x0C10},
    {0x10F5, 0x0001},
    {0x10F9, 0x0078},
    {0x10F7, 0x1010},
    {0x10F5, 0x0001},
    {0x10F9, 0x0078},
    {0x10F7, 0x1410},
    {0x10F5, 0x0001},
    {0x10F4, 0x0000},
    {0x10F9, 0x0000},
    {0x10F7, 0x0000},
    {0x1067, 0x050D},
    {0x1079, 0x4239},
    {0x1082, 0x4239},
    {0x1084, 0x5FFF},
    {0x1085, 0x5FFF},
    {0x1091, 0x4239},
    {0x1093, 0x4A39},
    {0x1065, 0x0006},
    {0x7069, 0x0187},
};

Hdmi_gs12170::Hdmi_gs12170(const char *spi_path){
    qCDebug(category) << "initialization gs12170 on spi_path" << spi_path;

    gs12170_spi = new Spi(spi_path, SPI_MODE_0);
    lock = lock_prev = 0;
    rate = rate_prev = 0;
    timer_update_gs12170 = new QTimer;
    connect(timer_update_gs12170, SIGNAL(timeout()), this, SLOT(slot_update_gs12170()));
    timer_update_gs12170->setInterval(500);
    timer_update_gs12170->start();

    init_gs12170();
}

Hdmi_gs12170::~Hdmi_gs12170(){
    delete gs12170_spi;
}

void Hdmi_gs12170::gs12170_write(uint16_t addr, uint16_t data){
    uint8_t tx_buff[GS12170_SPI_TRANSACTION_BYTES], rx_buff[GS12170_SPI_TRANSACTION_BYTES];

    uint16_t adr_emem = ADR_PLUS_EMEM;
    tx_buff[0]  = (uint8_t)(adr_emem >> 8);
    tx_buff[1]  = (uint8_t)adr_emem;
    tx_buff[2]  = (uint8_t)(addr >> 8);
    tx_buff[3]  = (uint8_t)addr;
    tx_buff[4]  = (uint8_t)(data >> 8);
    tx_buff[5]  = (uint8_t)data;

    int fd = gs12170_spi->open_device();
    gs12170_spi->transfer(fd, GS12170_SPI_TRANSACTION_BYTES, tx_buff, rx_buff);
    gs12170_spi->close_device(fd);
}

void Hdmi_gs12170::gs12170_read(uint16_t addr, uint16_t *data){
    uint8_t tx_buff[GS12170_SPI_TRANSACTION_BYTES], rx_buff[GS12170_SPI_TRANSACTION_BYTES];

    uint16_t adr_emem = ADR_PLUS_EMEM | 1 << 15;
    tx_buff[0]  = (uint8_t)(adr_emem >> 8);
    tx_buff[1]  = (uint8_t)adr_emem;
    tx_buff[2]  = (uint8_t)(addr >> 8);
    tx_buff[3]  = (uint8_t)addr;
    tx_buff[4]  = 0xFF;
    tx_buff[5]  = 0xFF;

    int fd = gs12170_spi->open_device();
    gs12170_spi->transfer(fd, GS12170_SPI_TRANSACTION_BYTES, tx_buff, rx_buff);
    gs12170_spi->close_device(fd);

    *data = rx_buff[4] << 8 | rx_buff[5];
}

void Hdmi_gs12170::init_gs12170(){
    for (int i = 0; i < sdi_hdmi.size(); i++) gs12170_write(sdi_hdmi[i].addr, sdi_hdmi[i].value);   //init tab sdi->hdmi mode
    gs12170_write(GS12170_ISP_REG, (1 << GS12170_ISP0_POS));	//inversion ISP0 polarity
	gs12170_write(GS12170_PMA_TX_REG58, 0x00FF);	            //inversion all out ports
    gs12170_write(GS12170_SDI_AUDIO_EN_REG, 0x0007);	        //AUD_EXT_EN_OVRD_EN, AUD_EXT_1_EN, AUD_EXT_2_EN
}

void Hdmi_gs12170::slot_update_gs12170(){
    uint16_t data;

    gs12170_read(GS12170_INPUT_LOCK_REG, &data);
	lock = data & GS12170_INPUT0_LOCK_MASK;
    // qDebug() << "GS12170_INPUT_LOCK_REG" << hex << data;
	gs12170_read(GS12170_DATA_RATE_REP_REG, &data);
	rate = data & GS12170_INPUT0_RATE_MASK;
    // qDebug() << "GS12170_DATA_RATE_REP_REG" << hex << data;

    if ((lock != lock_prev) || (rate != rate_prev)){
        emit gs12170_lock_rate_changed(rate);
        rate_prev = rate;
        lock_prev = lock;
    }

    // gs12170_read(GS12170_PMA_TX_REG58, &data);
    // qDebug() << "GS12170_PMA_TX_REG58" << hex << data;
    // gs12170_read(GS12170_SDI_AUDIO_EN_REG, &data);
    // qDebug() << "GS12170_SDI_AUDIO_EN_REG" << hex << data;
}