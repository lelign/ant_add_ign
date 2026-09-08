#ifndef CTRL_PANEL_PERP4116_H
#define CTRL_PANEL_PERP4116_H

#include <QObject>
#include <QDebug>
#include <QLoggingCategory>

#include "../vgpi_server/vgpi_server.h"
#include "../vgpi_api/vgpi_api.h"

#define SizeOfArray(a) (sizeof(a)/sizeof(*a))

class Ctrl_panel_PERP4116 : public QObject
{
    Q_OBJECT

#define PROTOCOL_VERSION_MAJOR 1
#define PROTOCOL_VERSION_MINOR 0
#define PROTOCOL_VERSION_REV   0

    enum {NO, YES};

    struct client_info_t{
        int wait_handshake;
        QString ip;
    };


public:
    explicit Ctrl_panel_PERP4116(int nPort);
    ~Ctrl_panel_PERP4116();

    enum ledColor_t{LED_OFF, LED_RED, LED_YELOW, LED_ORANGE};

    Vgpi_api   *vgpi_api;

    void set_led(uint ledNum, int state);
    void alive_answer(int type_specific);

private slots:
    void slot_parse_frame(int type, int BtnNum, int state);
    void slot_new_client(QTcpSocket* pSocket);
    void slot_readyRead(QTcpSocket* pSocket, QByteArray data);
    void slot_pDisconnected(QTcpSocket* pSocket);

signals:
    void signal_btn_pressed(int port, int BtnNum);
    void signal_btn_released(int port, int BtnNum);

private:
    Vgpi_server *vgpi_server;
    unsigned char led_state[5 + 1];
    QMap<QTcpSocket*, client_info_t> mClients;
    QByteArray handshakeBlock;
    int port;

    bool test_handshake(QTcpSocket* pSocket, QByteArray data);
    bool protocol_version_ok(QByteArray data);
    void set_led_state(QTcpSocket* pSocket);
};

#endif // CTRL_PANEL_PERP4116_H
