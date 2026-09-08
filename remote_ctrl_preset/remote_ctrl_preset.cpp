#include "remote_ctrl_preset.h"

#define PORT_1 10010
#define PORT_2 10011

static QLoggingCategory category("Remote Ctrl Preset");

Remote_ctrl_preset::Remote_ctrl_preset(QObject *parent) : QObject(parent)
{
    qCDebug(category) << "creating...";

    ctrl_panel_PERP4116_1 = new Ctrl_panel_PERP4116(10010);
    connect(ctrl_panel_PERP4116_1, &Ctrl_panel_PERP4116::signal_btn_pressed, this, &Remote_ctrl_preset::slot_Btn_pressed);

    ctrl_panel_PERP4116_2= new Ctrl_panel_PERP4116(10011);
    connect(ctrl_panel_PERP4116_2, &Ctrl_panel_PERP4116::signal_btn_pressed, this, &Remote_ctrl_preset::slot_Btn_pressed);
}

Remote_ctrl_preset::~Remote_ctrl_preset()
{
    delete  ctrl_panel_PERP4116_1;
}

void Remote_ctrl_preset::slot_Btn_pressed(int port, int BtnNum)
{
int layoutPresetNum = 0;

    if(port == PORT_1)
        layoutPresetNum = BtnNum - 1;

    if(port == PORT_2)
        layoutPresetNum = BtnNum - 1 + 5;

    set_colorLeds(layoutPresetNum + 1);
    qApp->processEvents();
    emit signal_set_preset_layout(layoutPresetNum);
    ctrl_panel_PERP4116_1->alive_answer(0);
}

void Remote_ctrl_preset::udate_colorLed(int layoutNum)
{
    set_colorLeds(layoutNum + 1);
}

void Remote_ctrl_preset::set_colorLeds(int ledToOn)
{
int state;

    for(int i = 1; i < 6; ++i){
        if(i == ledToOn)
            state = Ctrl_panel_PERP4116::LED_YELOW;
        else
            state = Ctrl_panel_PERP4116::LED_OFF;

        ctrl_panel_PERP4116_1->set_led(i, state);

        if(i == (ledToOn - 5))
            state = Ctrl_panel_PERP4116::LED_YELOW;
        else
            state = Ctrl_panel_PERP4116::LED_OFF;

        ctrl_panel_PERP4116_2->set_led(i, state);
    }
}


