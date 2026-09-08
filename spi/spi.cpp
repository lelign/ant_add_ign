#include "spi.h"

Spi::Spi(const char *spi_path, uint8_t spi_mode, QObject *parent) : QObject(parent){
    spi_device = spi_path;
    mode = spi_mode;
}


int Spi::open_spi(const char *spi_device,  uint8_t mode){
    int ret;
    int spi_fd;
    uint32_t speed = 1000000;

    spi_fd = open( spi_device, O_RDWR );

    if( spi_fd < 0 ) pabort( "No spi device" );

    ret = ioctl( spi_fd, SPI_IOC_WR_MODE, &mode );
    if( ret < 0 ) pabort("Error in mode");

    ret = ioctl( spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed );
    if( ret < 0 ) pabort( "Speed WR" );

    ret = ioctl( spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed );
    if( ret < 0 ) pabort( "Speed RX" );

    return spi_fd;
}


int Spi::open_device(){
    return open_spi(spi_device, mode);
}


void Spi::close_device(int fd){
        close(fd);
}


int Spi::transfer(int spi_fd, int len, uint8_t *tx, uint8_t *rx){
    int ret;
    struct spi_ioc_transfer tr;

    bzero(&tr, sizeof(spi_ioc_transfer));

    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    tr.len = len;

    ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
    if(ret < 0){
            qDebug() << "SPI transfer error";
    }

    return ret;
}


void Spi::pabort(const char *s){
    perror(s);
}

