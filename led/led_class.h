#ifndef LED_CLASS_H
#define LED_CLASS_H

#include <QObject>
#include <QFile>

class Led_class : public QObject
{
    Q_OBJECT
public:
    explicit Led_class(QObject *parent = nullptr);

void set_all_led_off();
void set_all_led_on();
void set_led_state(int num, int state);
private:
    int read_value(QString file_name);
    int intArg;
    void write_value(QString rewrite_line, int intArg);

signals:

public slots:

protected:
    void set_state(QString file_name, const char *state);

};

#endif // LED_CLASS_H
