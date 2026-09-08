#ifndef SPI_H
#define SPI_H

#include <QObject>
#include <QDebug>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <stdint.h>


class Spi : public QObject
{
    Q_OBJECT
public:
    explicit Spi(const char *spi_path, uint8_t spi_mode, QObject *parent = nullptr);
    int open_device();
    int  transfer(int spi_fd, int len, uint8_t *tx, uint8_t *rx);
    void close_device(int fd);

private:
    int  open_spi(const char *spi_device,  uint8_t mode);
    void pabort(const char *s);
    const char *spi_device;
    uint8_t mode;

};

#endif // SPI_H
