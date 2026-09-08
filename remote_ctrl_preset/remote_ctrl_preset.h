#ifndef REMOTE_CTRL_PRESET_H
#define REMOTE_CTRL_PRESET_H

#include <QObject>
#include <QDebug>
#include <QCoreApplication>
#include <QAbstractEventDispatcher>

#include "../ctrl_panel_PERP4116/ctrl_panel_perp4116.h"

class Remote_ctrl_preset : public QObject
{
    Q_OBJECT
public:
    explicit Remote_ctrl_preset(QObject *parent = nullptr);
    ~Remote_ctrl_preset();

    void udate_colorLed(int layoutNum);

signals:
    void signal_set_preset_layout(int layoutNum);

private:
    Ctrl_panel_PERP4116  *ctrl_panel_PERP4116_1;
    Ctrl_panel_PERP4116  *ctrl_panel_PERP4116_2;

public slots:
    void slot_Btn_pressed(int port, int BtnNum);
    void set_colorLeds(int ledToOn);

};

#endif // REMOTE_CTRL_PRESET_H
