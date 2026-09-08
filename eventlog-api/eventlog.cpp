#include "eventlog.h"
#include <QLocalSocket>

#define SERVER_NAME "/tmp/event_log_server"

void Eventlog::add(int category, QString message)
{
        QString str = QString::number(category) + ", " + message;

        QLocalSocket local_socket;
        local_socket.connectToServer(SERVER_NAME);
        if(local_socket.waitForConnected(50)){
                local_socket.write(str.toUtf8());
                local_socket.disconnectFromServer();
                local_socket.waitForDisconnected(50);
        }
}

Eventlog::Eventlog()
{

}

Eventlog::~Eventlog()
{

}