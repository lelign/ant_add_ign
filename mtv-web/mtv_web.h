#ifndef MTV_WEB_H
#define MTV_WEB_H

#include <QObject>
#include <QJsonArray>
#include <QNetworkInterface>
#include <QRegularExpression>

#include "../mtv-system/mtv-system.h"
#include "../web_socket_server/websocketserver.h"
#include "../hardware_diagnostics/hardware_diagnostics.h"
#include "../layout/layout.h"
#include "../uptime-utils/uptime_utils.h"
#include "../utils/utils.h"
#include "../dns-utils/dns-utils.h"
#include "../ntpdate-utils/ntpdate-utils.h"
#include "../eventlog-api/eventlog.h"
#include "../tsl/tsl-server.h"
#include "../remote_ctrl_preset/remote_ctrl_preset.h"
#include "../version.h"

class Mtv_web : public QObject
{
    Q_OBJECT
public:
    explicit Mtv_web(PbxMtvSystem *mtvsystem, Hardware_diagnostics *hardware_diagnostics, Layout *layout);
    /*explicit Mtv_web(PbxMtvSystem *mtvsystem, Hardware_diagnostics *hardware_diagnostics);*/
    ~Mtv_web();


    WebSocketServer *web_server;
    Eventlog        *eventlog;

    QJsonArray get_json_module_sdi_format();

signals:
    void signal_reconfigure();

public slots:
    void slot_web_new_client( QWebSocket *pClient );
    void slot_web_message(QWebSocket *pClient, QJsonObject obj);
    void slot_new_format();
    void slot_hardware_state(QString str_power_W, QString str_temperature_C);
    void slot_fan_state(int fanIsOk);
    void slot_set_cascade_data(int index);
    void slot_cascade_server_readyRead(QByteArray arr);
    void slot_cascade_update_sdi_format();
    void slot_cascade_slave_data_receive(int cascade_index, QByteArray data);
    void slot_tls_message(int addr, int tls, QString txt);
    void slot_TLS_TimeCounterCtrl(int addr, int tally, QString txt);
    void slot_setPreset(int presetIndex);

private:
    bool MTV_PBX_5161 = true;
    PbxMtvSystem         *mtvsystem;
    Hardware_diagnostics *hardware_diagnostics;
    Layout               *layout;
    Uptime_utils         *uptime_utils;
    TslServer            *tsl_server;
    Remote_ctrl_preset   *remote_ctrl_preset;

    QString version_software, build_id;
    QString power_W, temperature_C;
    QString fan_state;
    QString ntp_server, time_zone, dns_name;
    int     model_device;
    int     cout_cascade_answer[4];
    int rand_value;

    struct net_setting_t{
        QString ip;
        QString mask;
        QString gw;
        QString mac;
    };

    net_setting_t network_0;

    QByteArray  get_json_block_configuration();
    QJsonObject get_json_system();
    QByteArray get_json_layout_presets();
    QJsonObject get_json_hardware_diagnostics();
    QJsonArray  get_json_layout_video_list();
    QJsonArray  get_json_labels_list();
    QJsonObject get_json_layout_video_object(int i);
    QJsonObject get_json_labels_object(int i);
    QJsonObject get_json_solo();
    QJsonObject get_json_alarm_settings();
    QJsonObject get_json_network_setting();
    QJsonObject get_json_alarm_audio_settings();
    QJsonObject get_json_alarm_video_settings();
    QJsonObject get_json_layout();
    QJsonObject get_json_layout_clock();
    QJsonObject get_json_layout_time_counter();
    QJsonArray  get_json_layout_audio_alarm();
    QJsonArray  get_json_alarm_channel_enable(int index);
    QJsonArray  get_json_layout_video_alarm();
    QJsonArray  get_json_scte_104_display();
    QJsonArray  get_json_aspect_ratio_sd();
    QJsonArray  get_json_cell_style();
    QJsonArray  get_json_sdi_format_display();
    QJsonArray  get_json_umd_display();
    QJsonArray  get_json_sdi_label();
    QByteArray  get_json_settings();
    QJsonArray  get_json_sdi_format();
    QJsonArray  get_json_layout_preset_name();
    QString     read_preset_file(int file_num);
    QString     get_preset_file_name(int file_num);
    void write_preset_file(int file_num, QString text);

    void get_network_setting();

    void cmd_set_config(QJsonObject data_obj);
    void cmd_set_solo(QJsonObject data_obj);
    void cmd_set_preset(QJsonObject data_obj);
    void cmd_get_layout_presets(QWebSocket *pClient);
    void cmd_set_time(QJsonObject data_obj);
    void cmd_set_access_word(QJsonObject data_obj);
    void cmd_set_nework_settings(QJsonObject data_obj);
    void cmd_set_layout_presets(QJsonObject data_obj);
    void Settings_Read();
    void Check();
    void Settings_Write();
    void set_sys_conf();
    int  get_model_device();
    int  get_value(QString file_name);
    void slot_set_solo(Layout::solo_mode_t solo_mode);
    void parser_layout_JsonObject(QJsonObject layout_object);
    void parser_audio_alarm_channel_enable(QJsonObject video_obj, int index);
    void parser_sdi_input_label(QJsonArray jsonArray);
    void parser_preset_name(QJsonArray preset_name_arr);
    void parser_preset_files(QJsonArray preset_files_arr);
    void debugPrintJson(QString str, QByteArray data);
    void apply_new_conig();
    void check_sdi_format_from_slave();
    void clear_sdi_format_str(int cascade_index);
    void update_preset();
};

#endif // MTV_WEB_H
