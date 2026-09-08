#include "ctrl_panel_perp4116.h"

static QLoggingCategory category("Ctrl Panel perp-4116");

Ctrl_panel_PERP4116::Ctrl_panel_PERP4116(int nPort)
{
    port = nPort;

    handshakeBlock.resize(8);
    handshakeBlock[0] = 'P';
    handshakeBlock[1] = 'E';
    handshakeBlock[2] = 'R';
    handshakeBlock[3] = 'P';
    handshakeBlock[4] = 0x00;
    handshakeBlock[5] = PROTOCOL_VERSION_MAJOR;
    handshakeBlock[6] = PROTOCOL_VERSION_MINOR;
    handshakeBlock[7] = PROTOCOL_VERSION_REV;

    vgpi_api = new Vgpi_api;
    connect(vgpi_api, &Vgpi_api::signal_parse_frame, this, &Ctrl_panel_PERP4116::slot_parse_frame);

    vgpi_server = new Vgpi_server(nPort);
    connect(vgpi_server, &Vgpi_server::signal_new_client,    this, &Ctrl_panel_PERP4116::slot_new_client);
    connect(vgpi_server, &Vgpi_server::signal_readyRead,     this, &Ctrl_panel_PERP4116::slot_readyRead);
    connect(vgpi_server, &Vgpi_server::signal_pDisconnected, this, &Ctrl_panel_PERP4116::slot_pDisconnected);

    for(uint i = 0; i < SizeOfArray(led_state); i++) led_state[i] = LED_OFF;
}

Ctrl_panel_PERP4116::~Ctrl_panel_PERP4116()
{
        delete vgpi_api;
}


void Ctrl_panel_PERP4116::slot_parse_frame(int type, int BtnNum, int state)
{
    if(type == VGPI_TYPE_GET)
        alive_answer(BtnNum);

    if(type == VGPI_TYPE_SET){
        if(state)
            emit signal_btn_pressed(port, BtnNum);
        else
            emit signal_btn_released(port, BtnNum);
    }
}


void Ctrl_panel_PERP4116::alive_answer(int type_specific)
{
QByteArray arr;

    if(type_specific != 0) return;
    arr = vgpi_api->vgpi_frame_get_answer(0, 0);
    vgpi_server->sendToAllConnections(arr);
}


void Ctrl_panel_PERP4116::slot_new_client(QTcpSocket* pSocket)
{
QHostAddress peer_ip;
int peer_port;

    peer_ip = pSocket->peerAddress();
    peer_port = pSocket->localPort();

    qDebug() << "Connected ip:" << peer_ip.toString() << "port:" << peer_port;

    client_info_t client_info;
    client_info.wait_handshake = YES;
    client_info.ip             = peer_ip.toString();

    mClients[pSocket] = client_info;

    vgpi_server->sendClient(pSocket, handshakeBlock);
}

void Ctrl_panel_PERP4116::slot_readyRead(QTcpSocket* pSocket, QByteArray data)
{
    if(test_handshake(pSocket, data))
        vgpi_api->parse_frame(data);
}

void Ctrl_panel_PERP4116::slot_pDisconnected(QTcpSocket* pSocket)
{
    qDebug() << "disconnected:" << mClients[pSocket].ip;
    mClients.remove(pSocket);
}

void Ctrl_panel_PERP4116::set_led(uint ledNum, int state)
{
    if(ledNum >= SizeOfArray(led_state)) return;
    led_state[ledNum] = state;

    QByteArray arr = vgpi_api->vgpi_frame_uint16(ledNum, state);
    vgpi_server->sendToAllConnections(arr);
}

void Ctrl_panel_PERP4116::set_led_state(QTcpSocket* pSocket)
{
    QByteArray arr;
    for(int i = 1; i < 6; i++){
        arr = vgpi_api->vgpi_frame_uint16( i, led_state[i]);
        vgpi_server->sendClient(pSocket, arr);
    }
}


bool Ctrl_panel_PERP4116::test_handshake(QTcpSocket* pSocket, QByteArray data)
{
    if(mClients[pSocket].wait_handshake == 0) return true;

    mClients[pSocket].wait_handshake = NO;
    if(protocol_version_ok(data)){
        set_led_state(pSocket);
        return false;
    }

    vgpi_server->slot_close_connection(pSocket);
    return false;
}


bool Ctrl_panel_PERP4116::protocol_version_ok(QByteArray data)
{
bool result;

    result = (data == handshakeBlock);

    if(!result)
            qDebug() << "Incorrect connection protocol!";

    return result;
}

