#ifndef GPIO_H
#define GPIO_H

#include <QLoggingCategory>
#include <QObject>
#include <QTimer>
#include <QFile>

class Gpio: public QObject
{
    Q_OBJECT

public:
    enum gpio_mode_t{SOLO, TALLY, PRESET};

    Gpio();
    void set_common_alarm(int common_alarm);
    void set_mode(int mode);

public slots:
    void slot_update_SOLO_state();
    void slot_update_TALLY_state();
    void slot_update_PRESET_state();
    void slot_update_time_counter();
    void slot_update_solo_mode_desebled();
    void slot_update_state();
    // void slot_led_hps_a(); // ign
    void slot_led_hps_b(); // ign

signals:
    void signal_solo(int input_number);
    void signal_preset(int preset_number);
    void signal_solo_mode_desebled();
    void signal_TALLY(int input, int state);
    void signal_time_count_stop();
    void signal_time_count_start();

private:
    typedef struct{
        int old_state;
        QString path_to_gpio;
    } input_t;

    QList<input_t> input;
    input_t input_SOLO_desable;
    input_t input_time_counter;

    QTimer *timer_gpio_update;
    // QTimer *timer_led_hps_a; // ign
    QTimer *timer_led_hps_b; // ign
    int  old_common_alarm;
    int  gpio_mode;
    int  get_value(QString file_name);
    void set_state(QString file_name, const char *state);
    bool show_err = true;

};

#endif // GPIO_H
