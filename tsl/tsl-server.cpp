#include "tsl-server.h"

static QLoggingCategory category("TslServer");

TslServer::TslServer()
{
        qDebug(category) << "Creating...";

        socket = new QUdpSocket(this);
        socket->bind(QHostAddress::Any, 15000);
        connect(socket, &QUdpSocket::readyRead, this, &TslServer::dataready);
}

TslServer::~TslServer()
{
        delete socket;
}

void TslServer::dataready()
{
        while(socket->hasPendingDatagrams()){
                QByteArray datagram;
                datagram.resize(socket->pendingDatagramSize());
                socket->readDatagram(datagram.data(), datagram.size());
                process(datagram);
        }
}

void TslServer::process(QByteArray data)
{
        if((data.size() % 18) !=0)
                return;
        for(int i=0; i<data.size(); i+=18){
                process_tsl(data.mid(i, 18));
        }
}

void TslServer::process_tsl(QByteArray data)
{
        int addr = data[0] & 0x7f;
        int tally_addr = data[1];
        QString txt = data.mid(2);
        emit message(addr, tally_addr, txt.trimmed());
}
