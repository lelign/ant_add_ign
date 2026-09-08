#include <QLoggingCategory>
#include "hardware_diagnostics.h"

#define PATH_TO_FAN_STATE_A       ("/sys/class/gpio/gpio544/value") // ign
#define PATH_TO_FAN_STATE_B       ("/sys/class/gpio/gpio545/value") // ign
#define PATH_TO_FAN_STATE_RESET ("/sys/class/gpio/gpio546/value") // ign

#define PATH_TO_FILE_POWER_CONSUMPTION ("/sys/class/hwmon/hwmon2/power1_input") // ign 14247120 (5161) = 142.97 ! =>  + 15000 / 20000000
#define PATH_TO_FILE_NAME_TEMP         ("/sys/class/hwmon/hwmon0/temp1_input") // ign


/*
 PATH_TO_FAN_STATE_RESET = "/sys/class/gpio/gpio546/value" (out)
            PATH_TO_FAN_STATE
                PATH_TO_FAN_STATE_A =  "/sys/class/gpio/gpio544/value" (in)
                PATH_TO_FAN_STATE_B =  "/sys/class/gpio/gpio545/value" 

*/           

static QLoggingCategory category("Hardware_diagnostics Class");

Hardware_diagnostics::Hardware_diagnostics(QObject *parent) : QObject(parent)
{
    timer_info_update = new QTimer;
    connect(timer_info_update, SIGNAL(timeout()), this, SLOT(slot_timer_info_update()));
    timer_info_update->setInterval(3000);
    timer_info_update->start();
}

void Hardware_diagnostics::slot_timer_info_update()
{
    fan_state();

    int temperature_C = get_temperature_C();
    QString str_temperature_C = int_to_str(temperature_C);

    // Temperature control
    temperature_control(temperature_C);

    int power_W = get_power_W();
    QString str_power_W = int_to_str(power_W);

    emit signal_hardware_state(str_power_W, str_temperature_C);
}


void Hardware_diagnostics::temperature_control(int temperature)
{
#define TEMPER_HIGH_LIMIT (60 * 10)
#define TEMPER_LOW_LIMIT  (TEMPER_HIGH_LIMIT - (5 * 10))
static int temperature_over = 0;

    if(temperature > TEMPER_HIGH_LIMIT){
        if(temperature_over == 0){
            emit signal_over_temperature("Device temperature above 60°C");
            temperature_over = 1;
        }
    }
    if(temperature < TEMPER_LOW_LIMIT ) temperature_over = 0;
}

QString Hardware_diagnostics::int_to_str(int val)
{
    float d = (float)val / 10 ;
    return QString("%1").arg(d, 0, 'f', 1);
}


int Hardware_diagnostics::get_temperature_C()
{
    return get_value(PATH_TO_FILE_NAME_TEMP) / 100;
}


int Hardware_diagnostics::get_power_W()
{
    return (get_value(PATH_TO_FILE_POWER_CONSUMPTION) + 50000) / 100000;
}

void Hardware_diagnostics::fan_state()
{
static int fan_1_IsOk = -1;
static int fan_2_IsOk = -1;
int fan_1_tmp;
int fan_2_tmp;
    #if (BOARD_REV==1)
        return;
    #endif
    fan_1_tmp = get_value(PATH_TO_FAN_STATE_A);
    fan_2_tmp = get_value(PATH_TO_FAN_STATE_B);
    reset_fan_state();

    if(fan_1_IsOk != fan_1_tmp){
        fan_1_IsOk = fan_1_tmp;
        emit signal_fan_state(fan_1_IsOk);
    }
    if(fan_2_IsOk != fan_2_tmp){
        fan_2_IsOk = fan_2_tmp;
        emit signal_fan_state(fan_2_IsOk);
    }
}

int Hardware_diagnostics::get_value(QString file_name)
{
QString line;
int value;
    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly)){
        qDebug(category) << "Could not open file" << file_name;
        return -1;
    }

    line = file.readAll();
    file.close();

    value = line.toInt();

    return value;
}

void Hardware_diagnostics::reset_fan_state()
{
    #if (BOARD_REV==1)
        return;
    #endif
    set_state(PATH_TO_FAN_STATE_RESET, "1");
    set_state(PATH_TO_FAN_STATE_RESET, "0");
}


void Hardware_diagnostics::set_state(QString file_name, const char *state)
{
    QFile file(file_name);
    if(!file.open(QIODevice::ReadWrite)){
        qDebug(category) << "Could not open file" << file_name;
    }
    file.write(state);
    file.close();
}
