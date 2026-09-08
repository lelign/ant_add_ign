#ifndef EVENTLOG_H
#define EVENTLOG_H

#include <QtCore/QObject>

class Eventlog : public QObject
{
        Q_OBJECT
public:
    enum EVENT_CATEGORY{SYSTEM, CONNECTION, WARNING, CONTROL, INPUT_STATE, ERROR};


    void add(int category, QString message);
     Eventlog();
     ~Eventlog();
};


#endif
