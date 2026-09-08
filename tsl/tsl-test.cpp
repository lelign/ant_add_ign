#include "tsl-test.h"

TslTest::TslTest()
{
        server = new TslServer;
        connect(server, &TslServer::message, this, &TslTest::message);
}

TslTest::~TslTest()
{
        delete server;
}

void TslTest::message(int addr, int tally, QString txt)
{
        qDebug() << "Message" << addr << tally << txt;
}
