#include "cascade_ctrl.h"

#define DO_CONNECT_TIME 5000

static QLoggingCategory category("cascade_ctrl");

Cascade_ctrl::Cascade_ctrl(int index, QString dst_ip, int dst_port, int enable)
{
    qCDebug(category).noquote() << QString("Cascade %1. dst_ip = %2. Creating...").arg(index).arg(dst_ip) << enable;

    dst.ip     = dst_ip;
    dst.port   = dst_port;
    dst.enable = enable;
    dst.index  = index;

    tcpSocket = new QTcpSocket(this);

    connect(tcpSocket, &QTcpSocket::disconnected, this, &Cascade_ctrl::slot_disconnected);
    connect(tcpSocket, &QTcpSocket::connected,    this, &Cascade_ctrl::slot_connected);
    connect(tcpSocket, &QTcpSocket::readyRead,    this, &Cascade_ctrl::slot_readyRead);

    in.setDevice(tcpSocket);

    timer_tcp_connect = new QTimer(this);
    connect(timer_tcp_connect, &QTimer::timeout,  this, &Cascade_ctrl::do_connect);
    timer_tcp_connect->setInterval(DO_CONNECT_TIME); /* время переподключения */
    do_connect();
}


Cascade_ctrl::~Cascade_ctrl()
{
    tcpSocket->close();
    delete tcpSocket;
    delete timer_tcp_connect;
}

void Cascade_ctrl::slot_disconnected()
{
    qCDebug(category) << "Cascade device disconnected ip =" << tcpSocket->peerName();
    do_connect();
}

void Cascade_ctrl::slot_connected()
{
    qCDebug(category) << "Connect to" << dst.ip << "Port =" <<dst.port;
    timer_tcp_connect->stop();

    emit signal_connected(dst.index);
}


int Cascade_ctrl::write(QByteArray arrBlock)
{
    if(!dst.enable) return 0;

    if(tcpSocket == nullptr)
        return 0;
    if( tcpSocket->state() != QAbstractSocket::ConnectedState)
        return 0;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << arrBlock;

    return tcpSocket->write(block);
}

void Cascade_ctrl::debugPrintJson(QString str, QByteArray data)
{
    QJsonParseError err;
    QJsonDocument saveDoc = QJsonDocument::fromJson(data, & err);
    qCDebug(category) << str << "\n" << saveDoc.toJson().toStdString().c_str();
}

void Cascade_ctrl::slot_readyRead()
{
QByteArray data;

    while(1){
        in.startTransaction();
        in >> data;

        if(!in.commitTransaction()) return;

        emit signal_data_receive(dst.index, data);
    }

}

void Cascade_ctrl::do_connect()
{
    if(!dst.enable) return;
    if( tcpSocket->state() == QAbstractSocket::ConnectedState) return;

    timer_tcp_connect->start();
    qCDebug(category) << "Do Connect to ip="<< dst.ip << "port =" << dst.port;
    tcpSocket->connectToHost(dst.ip, dst.port);
}


void Cascade_ctrl::update_config(QString dst_ip, int dst_port, int enable)
{
    dst.ip     = dst_ip;
    dst.port   = dst_port;
    dst.enable = enable;

    if(enable){
        do_connect();
        emit signal_connected(dst.index);
    }
    else{
        tcpSocket->close();
    }

}
