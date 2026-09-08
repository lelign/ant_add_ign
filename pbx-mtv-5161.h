#ifndef PBXMTV508_H
#define PBXMTV508_H

#include <QtCore/QObject>
#include <QDebug>
#include <QMetaType>
#include <QLoggingCategory>
#include "mtv-system/mtv-system.h"
#include "hdmi_adv7513/hdmi_adv7513.h"
#include "hdmi_out/hdmi_gs12170.h"
#include "hdmi_out/sn75dp159.h"
#include "layout/layout.h"
#include "mtv-web/mtv_web.h"
#include "hardware_diagnostics/hardware_diagnostics.h"
//#include "m26/mb86m26_control.h"
//#include "HlsServer/hlsserver.h"
#include "factory_defaults/factory_defaults.h"
#include "watchdog.h"
#include "gpio/gpio.h"
#include "eventlog-api/eventlog.h"
#include "file_handle_leaks/file_handle_leaks.h"
#include "scte_104/scte_104.h"
#include "mtv-snmp/mtv_snmp.h"
#include "teletext-decoder/teletext-decoder.h"
#include "profnext/profnext-frontpanel.h"

class PbxMtv508 : public QObject
{
    Q_OBJECT
public:
   //HlsServer       *hlsserver;
    explicit PbxMtv508(bool watchdog=0);
    ~PbxMtv508();
    Eventlog *eventlog;


public slots:
    void slot_reset_to_factory_settings();
    void slot_over_temperature(QString str);
    void slot_fan_state(int fan_state);
    void slot_scte_104_data(int channel, QByteArray data);
    void slot_op47_data(int channel, QByteArray data);
    void slot_op42_data(int channel, QByteArray data);
    void slot_too_many_open_files(QString str);
    void slot_scte_104_in(int channel);
    void slot_scte_104_out(int channel);
    void indexed_string_event(uint8_t idx, uint16_t val);
    void slot_web_reconfigure();
    void slot_txt_page(int page);

private:
    PbxMtvSystem    *mtvsystem;
    //Hdmi_adv7513    *hdmi_adv7513; // ign
    Hdmi_gs12170 *hdmi_gs12170[2];
    Sn75dp159 *hdmi_sn75dp159[2];
    Layout          *layout;
    Mtv_web         *mtv_web;
    Hardware_diagnostics *hardware_diagnostics;
    //mb86m26_control *m26_control;
    Factory_defaults *factory_defaults;
    Watchdog        *watchdog;
    Gpio            *gpio;
    File_handle_leaks *file_handle_leaks;
    Scte_104        *scte_104[16]; // Scte_104        *scte_104[8]; // ign
    Mtv_snmp        *mtv_snmp;
    TeletextDecoder *teletext_decoder;
    ProfnextFrontPanel *panel;
    void send_cascade_log(int category, QString str);

    void device_config();
};

#endif
