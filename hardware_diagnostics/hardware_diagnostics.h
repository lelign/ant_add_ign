#ifndef HARDWARE_DIAGNOSTICS_H
#define HARDWARE_DIAGNOSTICS_H

#include <QObject>
#include <QFile>
#include <QTimer>

class Hardware_diagnostics : public QObject
{
    Q_OBJECT
public:
    explicit Hardware_diagnostics(QObject *parent = nullptr);

signals:
    void signal_over_temperature(QString);
    void signal_power_W(QString);
    void signal_fan_state(int);
    void signal_hardware_state(QString str_power_W, QString str_temperature_C);

public slots:
    void slot_timer_info_update();

private:
    int get_value(QString file_name);
    QString int_to_str(int val);
    int get_power_W();
    int get_temperature_C();
    void fan_state();
    QTimer *timer_info_update;
    void temperature_control(int temperature);
    void reset_fan_state();
    void set_state(QString file_name, const char *state);

};

#endif // HARDWARE_DIAGNOSTICS_H
