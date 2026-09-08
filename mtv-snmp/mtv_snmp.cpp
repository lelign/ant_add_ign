#include "mtv_snmp.h"

#define SNMP_ID 12

static QLoggingCategory category("mtv-snmp");

Mtv_snmp::Mtv_snmp(QObject *parent) : QObject(parent)
{
    qDebug(category) << "Creating...";

    common_alarm  = 0;
    str_temperature_C = "--.-";

    snmp_agent = new Snmp_Agent;

    set_snmp_data();
}

Mtv_snmp::~Mtv_snmp()
{
    delete snmp_agent;
}

/*---------------------------------------------------------------------------*/
void Mtv_snmp::set_snmp_data()
{
    snmp_agent->set_device_ID(SNMP_ID);
    #if (BOARD_REV==0)
    snmp_agent->set_mib_element_str("1", "PBX-MTV-508"    );
    #else
    snmp_agent->set_mib_element_str("1", "PN-MTV-581"    );
    #endif
    snmp_agent->set_mib_element_int("2", common_alarm     );  /* read only */
    snmp_agent->set_mib_element_str("3", str_power_W      );  /* read only */
    snmp_agent->set_mib_element_str("4", str_temperature_C);  /* read only */
}
/*---------------------------------------------------------------------------*/
void Mtv_snmp::slot_set_common_alarm(int common_alarm)
{
    this->common_alarm  = common_alarm;
    set_snmp_data();
}
/*---------------------------------------------------------------------------*/
void Mtv_snmp::slot_set_hardware_state(QString str_power_W, QString str_temperature_C)
{
    this->str_power_W       = str_power_W;
    this->str_temperature_C = str_temperature_C;
    set_snmp_data();
}
