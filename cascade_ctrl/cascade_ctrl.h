#ifndef CASCADE_CTRL_H
#define CASCADE_CTRL_H

#include <QObject>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QLoggingCategory>
#include <QDebug>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDataStream>

class Cascade_ctrl : public QObject
{
    Q_OBJECT


    typedef struct {
        QString ip;
        int     port;
        int     index;
        int     enable;
    } dst_t;

public:
    explicit Cascade_ctrl(int index, QString dst_ip, int dst_port, int enable);
    ~Cascade_ctrl();

    int  write(QByteArray data);
    void update_config(QString dst_ip, int dst_port, int enable);

signals:
    void signal_data_receive(int index, QByteArray data);
    void signal_connected(int index);

public slots:
    void slot_connected();
    void slot_disconnected();
    void slot_readyRead();

private:
    QTcpSocket  *tcpSocket;
    QTimer      *timer_tcp_connect;
    QDataStream in;

    void debugPrintJson(QString str, QByteArray data);

    dst_t dst;
    void do_connect();
};

#endif // CASCADE_CTRL_H
