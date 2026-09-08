#include <QLoggingCategory>
#include "factory_defaults.h"

/*#define PATH_TO_RESET_BUTTON_FILE "/sys/class/gpio/gpio444/value"*/
//#define PATH_TO_RESET_BUTTON_FILE "/sys/class/gpio/gpio558/value" // ign 5161
#define PATH_TO_RESET_BUTTON_FILE "/var/volatile/RESET_BUTTON" // ign 5161

static QLoggingCategory category("Factory Defaults Class");

extern "C" {
#include "../ip-utils/ip-utils.h"
}

Factory_defaults::Factory_defaults(QObject *parent) : QObject(parent)
{
    qCDebug(category) << "Creating...";

    timer_reset_active = new QTimer;
    connect(timer_reset_active, SIGNAL(timeout()), this, SLOT(slot_timer_reset_active()));
    timer_reset_active->setInterval(1000);
    timer_reset_active->start();
    cnt_time = 0;
}

void Factory_defaults::slot_timer_reset_active()
{
    if(get_value(PATH_TO_RESET_BUTTON_FILE))
        cnt_time = 0;
    else
        cnt_time++;

    if(cnt_time > 10){
        cnt_time = 0;
        reset_to_factory_settings();
    }
}

/*---------------------------------------------------------------------------*/
void Factory_defaults::reset_to_factory_settings()
{
    unsigned char   inet_ip[4] = {192, 168,   0, 209};
    unsigned char inet_mask[4] = {255, 255, 255,   0};
    unsigned char   inet_gw[4] = {192, 168,   0,   1};
    write_config(inet_ip, inet_mask, inet_gw);

    emit signal_reset_to_factory_settings();
}

int Factory_defaults::get_value(QString file_name)
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

void Factory_defaults::set_state(QString file_name, const char *state)
{
    QFile file(file_name);
    if(!file.open(QIODevice::ReadWrite)){
        qDebug(category) << "Could not open file" << file_name;
    }
    file.write(state);
    file.close();
}
