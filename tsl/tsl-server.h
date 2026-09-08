#ifndef TSLSERVER_H
#define TSLSERVER_H

#include <QtCore/QObject>
#include <QUdpSocket>
#include <QString>
#include <QLoggingCategory>

class TslServer : public QObject
{
        Q_OBJECT
public:
        explicit TslServer();
        ~TslServer();
private:
        QUdpSocket * socket;
        void process(QByteArray data);
        void process_tsl(QByteArray data);
private slots:
        void dataready();
signals:
        void message(int addr, int tally_addr, QString txt);
};

#endif
