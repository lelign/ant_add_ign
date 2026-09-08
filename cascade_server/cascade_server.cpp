#include "cascade_server.h"

static QLoggingCategory category("Cascade_server");

Cascade_server::Cascade_server(int nPort, QWidget* pwgt) :
    QWidget(pwgt),
    clientConnections()
{
    qCDebug(category) << "creating...";

    pnt_tcpServer = new QTcpServer(this);
    if (!pnt_tcpServer->listen(QHostAddress::Any, nPort)) {

        qCDebug(category) << "Server Error. Unable to start the server:"
                          << pnt_tcpServer->errorString();
        pnt_tcpServer->close();
        return;
    }

    connect(pnt_tcpServer, &QTcpServer::newConnection, this, &Cascade_server::slotNewConnection);

    qCDebug(category) << "listen port" << nPort;
}

Cascade_server::~Cascade_server(){
    clientConnections.clear();
    pClientSocket->close();
    pnt_tcpServer->close();
    delete pnt_tcpServer;
    delete pClientSocket;
}

void Cascade_server::slotNewConnection()
{
    pClientSocket = pnt_tcpServer->nextPendingConnection();
    connect(pClientSocket, SIGNAL(disconnected()),
            pClientSocket, SLOT(deleteLater())
            );
    connect(pClientSocket, SIGNAL(readyRead()),
            this,          SLOT(slotReadClient())
           );
    connect(pClientSocket, SIGNAL(disconnected()),
            this,          SLOT(slotDisconnected())
            );

    in.setDevice(pClientSocket);

    QHostAddress peer_ip = pClientSocket->peerAddress();

    emit signal_new_client(pClientSocket, peer_ip.toString());
}

void Cascade_server::slotDisconnected()
{
    emit signal_pDisconnected(pClientSocket);

    pClientSocket = nullptr;
}

void Cascade_server::slotReadClient()
{
QByteArray data;

    while(1){
        in.startTransaction();
        in >> data;

        if(!in.commitTransaction()) return;

        emit signal_readyRead(pClientSocket, data);
    }
}

void Cascade_server::sendClient(QByteArray arrBlock)
{
    if(pClientSocket == nullptr)
        return;
    if( pClientSocket->state() != QAbstractSocket::ConnectedState)
        return;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << arrBlock;

    pClientSocket->write(block);
}

void Cascade_server::slot_close_connection(QTcpSocket* pSocket)
{
    pSocket->close();
}

void Cascade_server::closeAllConnections()
{
    if(pClientSocket == nullptr) return;
    pClientSocket->close();
}
