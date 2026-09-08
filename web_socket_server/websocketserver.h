#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer> // ign

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)

class WebSocketServer : public QObject
{
        Q_OBJECT
public:
    explicit WebSocketServer(quint16 port, bool debug = false, QObject *parent = Q_NULLPTR);
    ~WebSocketServer();

    void senddata(QWebSocket *pClient, QByteArray data);
    void sendall(QByteArray data);


Q_SIGNALS:
    void closed();

signals:
    void signal_web_new_client( QWebSocket *pSocket );
    void signal_web_message( QWebSocket *pSocket, QJsonObject val);

private Q_SLOTS:
    void onNewConnection();
    void parse_message(QWebSocket *pClient, QString data);
    void processTextMessage(QString message);
    void socketDisconnected();
    QByteArray get_alive();
    void slot_led_hps_a(); // ign

private:
        QWebSocketServer *m_pWebSocketServer;
        QList<QWebSocket *> m_clients;
        bool m_debug;
        QTimer *timer_led_hps_a; // ign
        int  get_value(QString file_name); // ign
        void set_state(QString file_name, const char *state); // ign
        void check_void(); // ign
        int timer_set = 500; // ign
        //void saveByteArraySecurely(); //ign
        QString fileName;
        bool show_err = true;
};

#endif
