#ifndef PSGPFRONTPANELTEST_H
#define PSGPFRONTPANELTEST_H

#include <QtCore/QObject>
#include <QDebug>
#include <QTimer>

#include "profnext-frontpanel.h"

#define TIMER_TEST_VAL  2000

class ProfnextTest : public QObject
{
    Q_OBJECT
public:
        explicit ProfnextTest();
        ~ProfnextTest();
private:
        ProfnextFrontPanel * panel;
        QTimer * timer;
private Q_SLOTS:
        void timer_timeout();
        void indexed_string_event(uint8_t idx, uint16_t val);
};

#endif
