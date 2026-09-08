#ifndef _Cascade_server_h_
#define _Cascade_server_h_

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QLoggingCategory>

class Cascade_server : public QWidget {
Q_OBJECT

public:
    Cascade_server(int nPort, QWidget* pwgt = 0);
    ~Cascade_server();
    void sendClient(QByteArray arrBlock);
    void closeAllConnections();

private:
    QTcpServer* pnt_tcpServer;
    QList<QTcpSocket *> clientConnections;
    QDataStream in;
    QTcpSocket* pClientSocket  = nullptr;

signals:
    void signal_new_client(QTcpSocket* pSocket, QString      ip);
    void signal_readyRead (QTcpSocket* pSocket, QByteArray data);
    void signal_pDisconnected(QTcpSocket* pSocket);

public slots:
    void slotNewConnection();
    void slotReadClient   ();
    void slotDisconnected();
    void slot_close_connection(QTcpSocket* pSocket);

};
#endif  //_Cascade_server_h_
