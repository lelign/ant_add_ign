#include "vgpi_server.h"

static QLoggingCategory category("Vgpi Server");

Vgpi_server::Vgpi_server(int nPort)
{
    qCDebug(category) << "creating...";

    tcpServer = new QTcpServer(this);

    if(!tcpServer->listen(QHostAddress::Any, nPort)) {

        qCDebug(category) << "Server Error. Unable to start the server:"
                          << tcpServer->errorString();
        tcpServer->close();
        return;
    }

    connect(tcpServer, &QTcpServer::newConnection, this, &Vgpi_server::slotNewConnection);

    qCDebug(category) << "listen";

}

Vgpi_server::~Vgpi_server()
{
    clientConnections.clear();

    tcpServer->close();
    delete  tcpServer;
}

void Vgpi_server::slotNewConnection()
{
    QTcpSocket* pClientSocket = tcpServer->nextPendingConnection();
    connect(pClientSocket, &QTcpSocket::disconnected, pClientSocket, &QTcpSocket::deleteLater);
    connect(pClientSocket, &QTcpSocket::readyRead,    this,          &Vgpi_server::slotReadClient);
    connect(pClientSocket, &QTcpSocket::disconnected, this,          &Vgpi_server::slotDisconnected);

    QHostAddress peer_ip = pClientSocket->peerAddress();

    clientConnections << pClientSocket;

    emit signal_new_client(pClientSocket);
}

void Vgpi_server::sendClient(QTcpSocket* pSocket, QByteArray arrBlock)
{
    pSocket->write(arrBlock);
}

void Vgpi_server::slotReadClient()
{
    QTcpSocket *pClientSocket = (QTcpSocket*)sender();
    QByteArray data = pClientSocket->readAll();

    emit signal_readyRead(pClientSocket, data);
}

void Vgpi_server::sendToAllConnections(QByteArray arrBlock)
{
    foreach(QTcpSocket *clientConnection, clientConnections) {
       clientConnection->write(arrBlock);
    }
}

void Vgpi_server::slotDisconnected()
{
    QTcpSocket* pClientSocket = (QTcpSocket*)sender();

    clientConnections.removeAll(pClientSocket);

    emit signal_pDisconnected(pClientSocket);
}

void Vgpi_server::slot_close_connection(QTcpSocket* pSocket)
{
    pSocket->close();
}
