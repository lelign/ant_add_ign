#ifndef FACTORY_DEFAULTS_H
#define FACTORY_DEFAULTS_H

#include <QObject>
#include <QFile>
#include <QTimer>

class Factory_defaults : public QObject
{
    Q_OBJECT
public:
    explicit Factory_defaults(QObject *parent = nullptr);

signals:
    void signal_reset_to_factory_settings();

public slots:
    void slot_timer_reset_active();

private:
    QTimer *timer_reset_active;
    void set_state(QString file_name, const char *state);
    int  get_value(QString file_name);
    void reset_to_factory_settings();
    int cnt_time;
};

#endif // FACTORY_DEFAULTS_H
