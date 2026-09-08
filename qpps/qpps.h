#ifndef QPPS_H
#define QPPS_H

#include <QtCore/QObject>
#include <QTimer>

QT_USE_NAMESPACE;

class QPps : public QObject
{
        Q_OBJECT
public:
        QPps();
        ~QPps();
private:
        QTimer timer;
        void arm_timer();
Q_SIGNALS:
        void pps();
private Q_SLOTS:
        void timeout();
};

#endif