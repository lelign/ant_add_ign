#ifndef MTV_SNMP_H
#define MTV_SNMP_H

#include <QObject>
#include <QLoggingCategory>

#include "../snmp_agent/snmp_agent.h"

class Mtv_snmp : public QObject
{
    Q_OBJECT
public:
    explicit Mtv_snmp(QObject *parent = nullptr);
    ~Mtv_snmp();

    Snmp_Agent  *snmp_agent;


signals:

public slots:
    void slot_set_hardware_state(QString str_power_W, QString str_temperature_C);
    void slot_set_common_alarm(int common_alarm);

private:
    void set_snmp_data();
    int common_alarm;
    QString str_power_W, str_temperature_C;
};

#endif // MTV_SNMP_H
