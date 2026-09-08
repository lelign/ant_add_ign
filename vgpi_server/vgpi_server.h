#ifndef VGPI_SERVER_H
#define VGPI_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QLoggingCategory>

class Vgpi_server : public QObject
{
    Q_OBJECT
public:
    explicit Vgpi_server(int nPort);
    ~Vgpi_server();

    void sendToAllConnections(QByteArray arrBlock);
    void sendClient(QTcpSocket* pSocket, QByteArray arrBlock);


private:
    QTcpServer *tcpServer;
    QList<QTcpSocket *> clientConnections;


signals:
    void signal_new_client(QTcpSocket* pSocket);
    void signal_readyRead (QTcpSocket* pSocket, QByteArray data);
    void signal_pDisconnected(QTcpSocket* pSocket);

public slots:
    void slotNewConnection();
    void slotReadClient   ();
    void slotDisconnected();
    void slot_close_connection(QTcpSocket* pSocket);

};

#endif // VGPI_SERVER_H
