#ifndef TSLTEST_H
#define TSLTEST_H

#include <QtCore/QObject>
#include <QDebug>
#include "tsl-server.h"

class TslTest : public QObject
{
        Q_OBJECT
public:
        explicit TslTest();
        ~TslTest();
private:
        TslServer * server;
public slots:
        void message(int addr, int tally, QString txt);
};

#endif
