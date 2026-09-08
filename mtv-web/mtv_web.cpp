#include <QLoggingCategory>
#include <cstdlib>
#include "mtv_web.h"

#define PORT 8080
#define SIZE_LAYOUT_PRESET_NAME 32
#define SIZE_CLOCK_CELL_LABEL   32
#define COUNT_CASCADE_ANSWER    3

#define SizeOfArray(a) (sizeof(a)/sizeof(*a))

#define SETTINGS_SYS_CONFIG_FILE_NAME ("pbx-mtv-5161_sys")
#if (BOARD_REV==0)
#define PATH_TO_MODEL_DEVICE_FILE "/var/volatile/MODEL_DEVICE"
// #define PATH_TO_MODEL_DEVICE_FILE "/sys/class/gpio/gpio445/value" // in 508
#else
#define PATH_TO_MODEL_DEVICE_FILE "/var/volatile/MODEL_DEVICE"
// #define PATH_TO_MODEL_DEVICE_FILE "/sys/class/gpio/gpio477/value" // in 508
#endif


#define get_int_value(  obj, name, d) if((obj).value(name).isDouble()) d = (obj).value(name).toInt()
#define get_str_value(  obj, name, d) if((obj).value(name).isString()) d = (obj).value(name).toString()
#define get_bool_value( obj, name, d) if((obj).value(name).isBool  ()) d = (obj).value(name).toBool()
#define get_Json_Array( obj, name, d) if((obj).value(name).isArray ()) d = (obj).value(name).toArray()
#define get_Json_object(obj, name, d) if((obj).value(name).isObject()) d = (obj).value(name).toObject()

static QLoggingCategory category("Mtv_web");

extern "C" {
#include "../profitt-security/profitt-security.h"
#include "../ip-utils/ip-utils.h"
}

//Mtv_web::Mtv_web(PbxMtvSystem *mtvsystem, Hardware_diagnostics *hardware_diagnostics, Layout *layout) :
//Mtv_web::Mtv_web(PbxMtvSystem *mtvsystem, Hardware_diagnostics *hardware_diagnostics) :
//    mtvsystem(mtvsystem), hardware_diagnostics(hardware_diagnostics), layout(layout)
    //mtvsystem(mtvsystem), hardware_diagnostics(hardware_diagnostics) // ign

Mtv_web::Mtv_web(PbxMtvSystem *mtvsystem, Hardware_diagnostics *hardware_diagnostics, Layout *layout) :
    mtvsystem(mtvsystem), hardware_diagnostics(hardware_diagnostics), layout(layout)
{
    qCDebug(category) << "Class creating...";

    fan_state     = "--";
    power_W       = "--.-";
    temperature_C = "--.-";

    eventlog = new Eventlog;

    for(uint i = 0; i < SizeOfArray(cout_cascade_answer); ++i){
        cout_cascade_answer[i] = COUNT_CASCADE_ANSWER;
    }

    remote_ctrl_preset = new Remote_ctrl_preset;
    connect(remote_ctrl_preset, &Remote_ctrl_preset::signal_set_preset_layout, this, &Mtv_web::slot_setPreset);

    uptime_utils = new Uptime_utils;
    tsl_server   = new TslServer;

    web_server   = new WebSocketServer(PORT, 0);

    connect(web_server, SIGNAL(signal_web_new_client(QWebSocket*)), this,
                        SLOT  (slot_web_new_client  (QWebSocket*)));
    connect(web_server, SIGNAL(signal_web_message   (QWebSocket*, QJsonObject)), this,
                        SLOT  (slot_web_message     (QWebSocket*, QJsonObject)));

    connect(mtvsystem, &PbxMtvSystem::signal_new_format, this, &Mtv_web::slot_new_format);

    connect(hardware_diagnostics, &Hardware_diagnostics::signal_hardware_state, this, &Mtv_web::slot_hardware_state);
    connect(hardware_diagnostics, &Hardware_diagnostics::signal_fan_state,      this, &Mtv_web::slot_fan_state);
    
    connect(layout, &Layout::signal_preset, this, &Mtv_web::slot_setPreset);
    connect(layout, &Layout::signal_solo, this, &Mtv_web::slot_set_solo);
    connect(layout, &Layout::signal_cascade_device_connected,    this, &Mtv_web::slot_set_cascade_data);
    connect(layout, &Layout::signal_cascade_server_readyRead,    this, &Mtv_web::slot_cascade_server_readyRead);
    connect(layout, &Layout::signal_cascade_device_data_receive, this, &Mtv_web::slot_cascade_slave_data_receive);
    
    connect(tsl_server, &TslServer::message, this, &Mtv_web::slot_tls_message);
    connect(tsl_server, &TslServer::message, this, &Mtv_web::slot_TLS_TimeCounterCtrl);

    Settings_Read();

    remote_ctrl_preset->udate_colorLed(layout->layout_preset.index);

    model_device = get_model_device();
    if (MTV_PBX_5161)
        model_device = 4; // ign for PBX-MTV-5161
    std::srand(std::time(nullptr));
    rand_value = std::rand();
    qDebug(category) << model_device << " rand_value : " << rand_value;
}

 Mtv_web::~Mtv_web()
 {
    delete uptime_utils;
    delete web_server;
    delete tsl_server;
    delete eventlog;
 }
/*---------------------------------------------------------------------------*/
int Mtv_web::get_model_device(){
    #if (BOARD_REV==0)
    qDebug(category) << "\n\t\tBOARD_REV : " << BOARD_REV
                     << "\n\t\tPATH_TO_MODEL_DEVICE_FILE : " << PATH_TO_MODEL_DEVICE_FILE
                     << "\n\t\tPATH_TO_MODEL_DEVICE_FILE : " << get_value(PATH_TO_MODEL_DEVICE_FILE);
    if(get_value(PATH_TO_MODEL_DEVICE_FILE))
        return 0;
    else
        return 1;
    #else
    if(get_value(PATH_TO_MODEL_DEVICE_FILE))
        return 2;
    else
        return 3;
    #endif
}
/*---------------------------------------------------------------------------*/
int Mtv_web::get_value(QString file_name)
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
/*---------------------------------------------------------------------------*/
void Mtv_web::slot_set_solo(Layout::solo_mode_t solo_mode){

    if((layout->solo_mode.input  == solo_mode.input)&&
       (layout->solo_mode.enable == solo_mode.enable))
        return;

    layout->solo_mode.input  = solo_mode.input;
    layout->solo_mode.enable = solo_mode.enable;

    layout->Settings_Write();

    layout->update_layout();

    QByteArray to_send_data = get_json_settings();
    web_server->sendall(to_send_data);
}
/*---------------------------------------------------------------------------*/
void Mtv_web::slot_fan_state(int fanIsOk)
{
    if(fanIsOk)
        fan_state = "Ok";
    else
        fan_state = "fault";
}
/*---------------------------------------------------------------------------*/
void Mtv_web::slot_hardware_state(QString str_power_W, QString str_temperature_C)
{
    power_W       = str_power_W;
    temperature_C = str_temperature_C;

    QByteArray to_send_data = get_json_block_configuration();
    web_server->sendall(to_send_data);

    slot_cascade_update_sdi_format();
    check_sdi_format_from_slave();
}
/*---------------------------------------------------------------------------*/
void Mtv_web::check_sdi_format_from_slave(){

    for(uint i = 0; i < SizeOfArray(cout_cascade_answer); ++i){
        cout_cascade_answer[i]--;
        if(cout_cascade_answer[i] == 0){
            cout_cascade_answer[i] = -1;
            clear_sdi_format_str(i);
        }

        if(cout_cascade_answer[i] < 0) cout_cascade_answer[i] = -1;
    }

}
/*---------------------------------------------------------------------------*/
void Mtv_web::clear_sdi_format_str(int cascade_index)
{
    for(int i = 0; i < 8; i++){
        int k = (cascade_index + 1) * 16 + i; // int k = (cascade_index + 1) * 8 + i; // ign for 5161
        // layout->layout_object[k].sdi_format_str = ""; ign commented
        if (k < cascade_index){ // ign added cause app failed
            layout->layout_object[k].sdi_format_str = "";
                /*
                qDebug(category) << " clear_sdi_format_str [k] " << k 
                << "\n\t\tlayout " << layout->layout_object[k].sdi_format_str
                << "\n\t\tcascade_index " << cascade_index;
                */
                
        }
       
        
    }
}
/*---------------------------------------------------------------------------*/
void Mtv_web::slot_new_format()
{
    qDebug(category) << "New SDI Input Format";

    QByteArray to_send_data = get_json_block_configuration();
    web_server->sendall(to_send_data);
}
/*---------------------------------------------------------------------------*/
void Mtv_web::slot_web_message(QWebSocket *pClient, QJsonObject obj)
{
QJsonObject data_obj;
QJsonValue val;

    val = obj.value("type");
    if(val.isString()){
        if(val.toString() == "set_config"){
            if(obj.value("data").isObject()){
                data_obj = obj.value("data").toObject();
                cmd_set_config(data_obj);
            }
            else{
                qDebug(category) << "slot_web_message() set_config Configuration not JsonObject";
            }
        }
        if(val.toString() == "set_solo"){
            if(obj.value("data").isObject()){
                data_obj = obj.value("data").toObject();
                cmd_set_solo(data_obj);
            }
            else{
                qDebug(category) << "slot_web_message() Configuration not JsonObject obj.value(data).isObject() : " 
                        << obj.value("data").isObject();
            }
        }
        if(val.toString() == "set_preset"){
            if(obj.value("data").isObject()){
                data_obj = obj.value("data").toObject();
                cmd_set_preset(data_obj);
            }
            else{
                qDebug(category) << "set_preset Configuration not JsonObject";
            }
        }
        if(val.toString() == "set_time"){
            if(obj.value("data").isObject()){
                data_obj = obj.value("data").toObject();
                cmd_set_time(data_obj);
            }
            else{
                qDebug() << "Configuration not JsonObject";
            }
        }
        if(val.toString() == "access"){
            if(obj.value("data").isObject()){
                data_obj = obj.value("data").toObject();
                cmd_set_access_word(data_obj);
            }
            else{
                qDebug() << "Configuration not JsonObject";
            }
        }
        if(val.toString() == "reboot"){
            system("reboot");
        }
        if(val.toString() == "get_layout_presets"){
            cmd_get_layout_presets(pClient);
        }
        if(val.toString() == "set_network_settings"){
            if(obj.value("data").isObject()){
                data_obj = obj.value("data").toObject();
                cmd_set_nework_settings(data_obj);
            }
            else{
                qDebug() << "Configuration not JsonObject";
            }
        }
        if(val.toString() == "set_layout_presets"){
            if(obj.value("data").isObject()){
                data_obj = obj.value("data").toObject();
                cmd_set_layout_presets(data_obj);
            }
            else{
                qDebug() << "Configuration not JsonObject";
            }
        }
    }
}
/*---------------------------------------------------------------------------*/
void Mtv_web::cmd_set_nework_settings(QJsonObject data_obj)
{
QRegularExpression re("(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)");
QString eth1_ip;
QString eth1_mask;
QString eth1_gw;

unsigned char   inet_ip[4] = {169, 254,   0, 209};
unsigned char inet_mask[4] = {255, 255, 255,   0};
unsigned char   inet_gw[4] = {169, 254,   0,   1};

    get_str_value(data_obj, "ip"     , eth1_ip  );
    get_str_value(data_obj, "mask"   , eth1_mask);
    get_str_value(data_obj, "gateway", eth1_gw  );

    QRegularExpressionMatch match_ip = re.match(eth1_ip);
    if(match_ip.hasMatch()){
        for(int i = 0; i < 4; i++)
            inet_ip[i] = match_ip.captured(i + 1).toInt();
    }
    QRegularExpressionMatch match_gw = re.match(eth1_gw);
    if(match_gw.hasMatch()){
        for(int i = 0; i < 4; i++)
            inet_gw[i] = match_gw.captured(i + 1).toInt();
    }
    QRegularExpressionMatch match_mask = re.match(eth1_mask);
    if(match_mask.hasMatch()){
        for(int i = 0; i < 4; i++)
            inet_mask[i] = match_mask.captured(i + 1).toInt();
    }
    qDebug(category) << "\tcmd_set_nework_settings" << eth1_ip << eth1_gw << eth1_mask << inet_ip, inet_mask, inet_gw ;
    write_config(inet_ip, inet_mask, inet_gw);
}
/*---------------------------------------------------------------------------*/
void Mtv_web::update_preset()
{
    layout->Settings_Write();
    layout->preset_Layout_Read(layout->layout_preset.index);
    layout->update_layout();

    QByteArray to_send_data = get_json_settings();
    web_server->sendall(to_send_data);
}
/*---------------------------------------------------------------------------*/
void Mtv_web::slot_setPreset(int presetIndex)
{
    if(layout->cascade.mode == Layout::CASCADE_SLAVE) return;

    layout->layout_preset.index = presetIndex;
    update_preset();
}
/*---------------------------------------------------------------------------*/
void Mtv_web::cmd_set_preset(QJsonObject data_obj)
{
    get_int_value(data_obj, "index", layout->layout_preset.index);
    remote_ctrl_preset->udate_colorLed(layout->layout_preset.index);
    update_preset();
}
/*---------------------------------------------------------------------------*/
void Mtv_web::cmd_get_layout_presets( QWebSocket *pClient )
{
    qDebug(category) << "Command get layout presets";
    
    
    QByteArray to_send_data = get_json_layout_presets();

    web_server->senddata( pClient, to_send_data );
}
/*---------------------------------------------------------------------------*/
void Mtv_web::cmd_set_access_word(QJsonObject data_obj)
{
QString s;

    get_str_value(data_obj, "codeword", s);

    secutiry_set_password(s.toStdString().c_str());
}
/*---------------------------------------------------------------------------*/
void Mtv_web::cmd_set_time(QJsonObject data_obj)
{
    time_t t = 0;
    get_int_value(data_obj, "time", t);
    mtvsystem->system_set_time(t);
}
/*---------------------------------------------------------------------------*/
void Mtv_web::cmd_set_solo(QJsonObject data_obj)
{
    Layout::solo_mode_t solo_mode;
    get_int_value(data_obj, "enable", solo_mode.enable);
    get_int_value(data_obj, "input",  solo_mode.input);

    bool need_solo_update = layout->solo_mode.enable != solo_mode.enable;

    if(solo_mode.enable && (layout->solo_mode.input != solo_mode.input))
            need_solo_update = true;

    if(need_solo_update)
        slot_set_solo(solo_mode);
}
/*---------------------------------------------------------------------------*/
void Mtv_web::set_sys_conf()
{
    qDebug() << "set_sys_conf";
    update_timezone(time_zone.toStdString().c_str());
    qDebug() << "update_timezone";
    update_resolvconf(dns_name.toStdString().c_str());
    qDebug() << "update_resolveconf";
    ntpdate_server(ntp_server.toStdString().c_str());
    qDebug() << "ntpdate_server";
}
/*---------------------------------------------------------------------------*/
void Mtv_web::cmd_set_config(QJsonObject data_obj)
{
QList<QJsonObject> video_obj;
QList<QJsonObject> audio_alarm_obj;
QJsonArray  jsonArray;
QJsonObject layout_object;
QJsonObject clock_object;

    get_str_value(data_obj,"ntp",       ntp_server);
    get_str_value(data_obj,"time_zone", time_zone);
    get_str_value(data_obj,"dns",       dns_name);
    Settings_Write();

    set_sys_conf();

    get_int_value(data_obj, "cascade_mode", layout->cascade.mode);
    get_int_value(data_obj, "gpio_mode",    layout->gpio_mode);

    for(uint i = 0; i < SizeOfArray(layout->cascade.ip); i++){
        QString ip_str   = "ip_" + QString::number(i + 2);
        get_str_value(data_obj, ip_str, layout->cascade.ip[i]);
    }

    get_Json_Array(data_obj, "sdi_input_label", jsonArray);
    parser_sdi_input_label(jsonArray);

    get_Json_object(data_obj, "layout", layout_object);
    parser_layout_JsonObject(layout_object);

    int index =  layout->layout_preset.index;
    get_str_value(layout_object, "label", layout->layout_preset.name[index]);
    if(layout->layout_preset.name[index].count() > SIZE_LAYOUT_PRESET_NAME)
        layout->layout_preset.name[index].truncate(SIZE_LAYOUT_PRESET_NAME);

    get_Json_object(layout_object, "clock", clock_object);
    get_int_value(clock_object, "enable",      layout->clock_cell.enable);
    get_int_value(clock_object, "scale_x",     layout->clock_cell.scale_x);
    get_int_value(clock_object, "scale_y",     layout->clock_cell.scale_y);
    get_int_value(clock_object, "x",           layout->clock_cell.x);
    get_int_value(clock_object, "y",           layout->clock_cell.y);
    get_int_value(clock_object, "style",       layout->clock_cell.style);
    get_str_value(clock_object, "label",       layout->clock_cell.label);
    get_int_value(clock_object, "date_locale", layout->clock_cell.date_locale);
    if(layout->clock_cell.label.count() > SIZE_CLOCK_CELL_LABEL)
        layout->clock_cell.label.truncate(SIZE_CLOCK_CELL_LABEL);

    get_Json_object(layout_object, "time_counter", clock_object);
    get_int_value(clock_object, "enable",  layout->time_counter_cell.enable);
    get_int_value(clock_object, "scale_x", layout->time_counter_cell.scale_x);
    get_int_value(clock_object, "scale_y", layout->time_counter_cell.scale_y);
    get_int_value(clock_object, "x",       layout->time_counter_cell.x);
    get_int_value(clock_object, "y",       layout->time_counter_cell.y);

    get_int_value  (layout_object, "teletext_enable", layout->teletext_cell.enable);
    get_int_value  (layout_object, "teletext_page",   layout->teletext_cell.page);

    QJsonObject alarm_settings_val;
    get_Json_object(data_obj, "alarm_settings", alarm_settings_val);
    QJsonObject audio_alarm_settings;
    get_Json_object(alarm_settings_val, "audio", audio_alarm_settings);
    get_int_value(audio_alarm_settings, "threshold",      layout->alarm_settings.audio_alarm_settings.threshold);
    get_int_value(audio_alarm_settings, "minimum_level",  layout->alarm_settings.audio_alarm_settings.minimum_level);
    if(layout->alarm_settings.audio_alarm_settings.minimum_level > -30) layout->alarm_settings.audio_alarm_settings.minimum_level = -30;
    if(layout->alarm_settings.audio_alarm_settings.minimum_level < -80) layout->alarm_settings.audio_alarm_settings.minimum_level = -80;
    QJsonObject video_alarm_settings;
    get_Json_object(alarm_settings_val, "video", video_alarm_settings);
    get_int_value(video_alarm_settings, "threshold",      layout->alarm_settings.video_alarm_settings.threshold);

    get_int_value(data_obj, "output_format", layout->output_format);
    get_int_value(data_obj, "hdmi_color", layout->hdmi_color);

    if(layout->cascade.mode == Layout::CASCADE_SLAVE){
        layout->teletext_cell.enable = 0;
        layout->query_cascade_data(); // запросить новые данные каскадирования
    }
    if(layout->cascade.mode == Layout::STAND_ALONE){
        layout->cascade.num = 0;
        layout->cascade.last_slave_device = 1;
    }
    else{
        layout->cascade.last_slave_device = 0;
    }

    apply_new_conig();
}
/*---------------------------------------------------------------------------*/
void Mtv_web::parser_sdi_input_label(QJsonArray jsonArray)
{
    int layout_object_size = SizeOfArray(layout->layout_object);
    for(int i = 0; i < jsonArray.size(); i++){
        if(i >= layout_object_size)
            break;
        layout->layout_object[i].sdi_label = jsonArray[i].toString();
    }
    qDebug(category) << "486 int layout_object_size : " << layout_object_size;
}
/*---------------------------------------------------------------------------*/
void Mtv_web::apply_new_conig()
{
    layout->Settings_Write();
    layout->preset_Layout_Write(layout->layout_preset.index);
    layout->update_layout();
    layout->slot_new_format();
    emit signal_reconfigure();

    QByteArray to_send_data = get_json_settings();
    qDebug(category) << "apply_new_config : " << to_send_data.size();
    web_server->sendall(to_send_data);
}
/*---------------------------------------------------------------------------*/
void Mtv_web::parser_audio_alarm_channel_enable(QJsonObject video_obj, int index)
{
    QJsonValue audio_alarm_channel_val = video_obj.value("audio_alarm_channel_enable");
    QJsonArray audio_alarm_channel_array = audio_alarm_channel_val.toArray();

    for(int i = 0; i < audio_alarm_channel_array.size(); i++){
        layout->layout_object[index].cell.audio_alarm_channel_enable[i] = audio_alarm_channel_array[i].toInt();
    }
}
/*---------------------------------------------------------------------------*/
void Mtv_web::parser_layout_JsonObject(QJsonObject layout_object)
{
QList<QJsonObject> video_obj;

    int layout_object_size = SizeOfArray(layout->layout_object);

    QJsonValue video_val = layout_object.value("video");
    QJsonArray video_array = video_val.toArray();
    for(int i = 0; i < video_array.size(); i++){
        if(i >= layout_object_size)
            break;
        video_obj.append( video_array[i].toObject());
    }

    for(int i = 0; i < video_obj.size(); i++){
        if(i >= layout_object_size)
            break;
        get_int_value(video_obj[i], "scale_x", layout->layout_object[i].cell.scale_x);
        get_int_value(video_obj[i], "scale_y", layout->layout_object[i].cell.scale_y);
        get_int_value(video_obj[i], "enable",  layout->layout_object[i].cell.enable);
        get_int_value(video_obj[i], "x",       layout->layout_object[i].cell.x);
        get_int_value(video_obj[i], "y",       layout->layout_object[i].cell.y);
        get_int_value(video_obj[i], "bars",    layout->layout_object[i].cell.audio_meter);
        parser_audio_alarm_channel_enable(video_obj[i], i);
    }

    QList<QJsonObject> labels_obj;
    QJsonValue labels_val = layout_object.value("labels");
    QJsonArray labels_array = labels_val.toArray();
    for(int i = 0; i < labels_array.size(); i++){
        if(i >= layout_object_size)
            break;
        labels_obj.append(labels_array[i].toObject());
    }

    for(int i = 0; i < labels_obj.size(); i++){
        if(i >= layout_object_size)
            break;
        get_int_value(labels_obj[i], "scale_x", layout->label_cell[i].scale_x);
        get_int_value(labels_obj[i], "scale_y", layout->label_cell[i].scale_y);
        get_int_value(labels_obj[i], "enable",  layout->label_cell[i].enable);
        get_int_value(labels_obj[i], "x",       layout->label_cell[i].x);
        get_int_value(labels_obj[i], "y",       layout->label_cell[i].y);
        get_str_value(labels_obj[i], "text",    layout->label_cell[i].label.text);
    }

    QJsonValue audio_alarm_val = layout_object.value("audio_alarm_enable");
    QJsonArray audio_alarm_array = audio_alarm_val.toArray();
    for(int i = 0; i < audio_alarm_array.size(); i++){
        if(i >= layout_object_size)
            break;
        layout->layout_object[i].cell.audio_alarm.enable = audio_alarm_array[i].toInt();
    }

    QJsonValue video_alarm_val = layout_object.value("video_alarm_enable");
    QJsonArray video_alarm_array = video_alarm_val.toArray();
    for(int i = 0; i < video_alarm_array.size(); i++){
        if(i >= layout_object_size)
            break;
        layout->layout_object[i].cell.video_alarm.enable = video_alarm_array[i].toInt();
    }

    QJsonValue scte_104_display_val = layout_object.value("scte_104_display");
    QJsonArray scte_104_display_array = scte_104_display_val.toArray();
    for(int i = 0; i < scte_104_display_array.size(); i++){
        if(i >= layout_object_size)
            break;
        layout->layout_object[i].cell.scte_104_display = scte_104_display_array[i].toInt();
    }

    QJsonValue aspect_ratio_sd_val = layout_object.value("aspect_ratio_sd");
    QJsonArray aspect_ratio_sd_array = aspect_ratio_sd_val.toArray();
    for(int i = 0; i < aspect_ratio_sd_array.size(); i++){
        if(i >= layout_object_size)
            break;
        layout->layout_object[i].cell.aspect_ratio_sd = aspect_ratio_sd_array[i].toInt();
    }

    QJsonValue cell_style_val = layout_object.value("cell_style");
    QJsonArray cell_style_array = cell_style_val.toArray();
    for(int i = 0; i < cell_style_array.size(); i++){
        if(i >= layout_object_size)
            break;
        layout->layout_object[i].cell.style = cell_style_array[i].toInt();
    }

    QJsonValue sdi_format_display_val = layout_object.value("sdi_format_display");
    QJsonArray sdi_format_display_array = sdi_format_display_val.toArray();
    for(int i = 0; i < sdi_format_display_array.size(); i++){
        if(i >= layout_object_size)
            break;
        layout->layout_object[i].cell.sdi_format_display = sdi_format_display_array[i].toInt();
    }

    QJsonValue umd_display_val = layout_object.value("umd_display");
    QJsonArray umd_display_array = umd_display_val.toArray();
    for(int i = 0; i < umd_display_array.size(); i++){
        if(i >= layout_object_size)
            break;
        layout->layout_object[i].cell.umd_display = umd_display_array[i].toInt();
    }

    get_int_value(layout_object, "grid",  layout->grid);
}
/*---------------------------------------------------------------------------*/
void Mtv_web::debugPrintJson(QString str, QByteArray data)
{
    QJsonParseError err;
    QJsonDocument saveDoc = QJsonDocument::fromJson(data, & err);
    qCDebug(category) << str << "\n" << saveDoc.toJson().toStdString().c_str();
}
/*---------------------------------------------------------------------------*/
void Mtv_web::slot_cascade_slave_data_receive(int cascade_index, QByteArray arr)
{
Q_UNUSED(cascade_index);
QJsonObject cascade_settings_obj;
QJsonObject layout_object;
QJsonArray  jsonArray;
int cascade_num = 0;

    cout_cascade_answer[cascade_index] = COUNT_CASCADE_ANSWER;

    QJsonDocument itemDoc = QJsonDocument::fromJson(arr);

    if(itemDoc.object().contains("cascade_status")){
        get_Json_object(itemDoc.object(), "cascade_status", cascade_settings_obj);
        get_int_value(cascade_settings_obj, "cascade_num",  cascade_num);

        get_Json_Array(cascade_settings_obj, "sdi_format", jsonArray);
    for(int i = 0; i < 16; i++){
            int k = cascade_num * 16 + i;
            layout->layout_object[k].sdi_format_str = jsonArray[i].toString();
            }
    }

    if(itemDoc.object().contains("set_solo")){
        get_Json_object(itemDoc.object(), "set_solo", cascade_settings_obj);
        cmd_set_solo(cascade_settings_obj);
    }

    if(itemDoc.object().contains("eventlog_cascade")){
        get_Json_object(itemDoc.object(), "eventlog_cascade", cascade_settings_obj);
        int category = 0;
        get_int_value(cascade_settings_obj, "category",  category);
        QString message_str;
        get_str_value(cascade_settings_obj, "message",  message_str);

        eventlog->add(category, message_str);
    }
}
/*---------------------------------------------------------------------------*/
void Mtv_web::slot_TLS_TimeCounterCtrl(int addr, int tally, QString txt)
{
Q_UNUSED(txt);
    if(addr != 50) return;

    if(tally)
        layout->timer_time_counter->slot_stop();
    else
        layout->timer_time_counter->slot_start();
}
/*---------------------------------------------------------------------------*/
void Mtv_web::slot_tls_message(int addr, int tally, QString txt)
{
QJsonObject json;
QJsonObject data_obj;

    if(addr >= (int)SizeOfArray(layout->layout_object)) return;

    data_obj["addr" ] = addr;
    data_obj["tally"] = tally;
    data_obj["txt"  ] = txt;

    json["tls_message"] = data_obj;

    QJsonDocument saveDoc(json);
    QByteArray arr = saveDoc.toJson();

    for(uint i = 0; i < SizeOfArray(layout->cascade_ctrl); i++){
        layout->cascade_ctrl[i]->write(arr);
    }

    layout->update_label_name_channel(addr, txt);
    layout->layout_object[addr].tally = tally;
    layout->DisplayTALLY(addr, tally);

    layout->Settings_Write();

    QByteArray to_send_data = get_json_settings();
    web_server->sendall(to_send_data);
}
/*---------------------------------------------------------------------------*/
void Mtv_web::slot_cascade_server_readyRead(QByteArray arr)
{
QJsonObject cascade_settings_obj;
QJsonObject layout_object;
QJsonObject solo_obj;
QJsonObject tls_message_obj;
QJsonArray  jsonArray;

    QJsonDocument itemDoc = QJsonDocument::fromJson(arr);

    if(itemDoc.object().contains("cascade_settings")){
        get_Json_object(itemDoc.object(), "cascade_settings", cascade_settings_obj);

        get_Json_object(cascade_settings_obj, "layout", layout_object);
        parser_layout_JsonObject(layout_object);

        get_Json_Array(cascade_settings_obj, "sdi_input_label", jsonArray);
        parser_sdi_input_label(jsonArray);

        get_int_value(cascade_settings_obj, "cascade_num",    layout->cascade.num);
        get_int_value(cascade_settings_obj, "sdi_out_format", layout->output_format);

        get_int_value(cascade_settings_obj, "last_slave_device", layout->cascade.last_slave_device);

        get_Json_object(cascade_settings_obj, "solo", solo_obj);
        cmd_set_solo(solo_obj);

        get_int_value(cascade_settings_obj, "gpio_mode", layout->gpio_mode);

        layout->clock_cell.enable = 0; // часы рисует только мастер

        apply_new_conig();

        return;
    }

    if(itemDoc.object().contains("tls_message")){
        int addr = 0, tally = 0;
        QString txt;

        get_Json_object(itemDoc.object(), "tls_message", tls_message_obj);

        get_int_value(tls_message_obj, "addr",  addr);
        get_int_value(tls_message_obj, "tally", tally);
        get_str_value(tls_message_obj, "txt",   txt);

        layout->layout_object[addr].tally = tally;

        layout->DisplayTALLY(addr, tally);

        layout->update_label_name_channel(addr, txt);

        return;
    }
}
/*---------------------------------------------------------------------------*/
QJsonArray Mtv_web::get_json_module_sdi_format()
{
QJsonArray sdi_input_arr;

    for(uint i = 0; i < 16 ; ++i){ //ign for(uint i = 0; i < 8 ; ++i){
        sdi_input_arr.append(mtvsystem->get_sdi_format_str(i));
    }
    
    return sdi_input_arr;
}
/*---------------------------------------------------------------------------*/
void Mtv_web::slot_cascade_update_sdi_format()
{
QJsonObject json;
QJsonObject data_obj;

    if(layout->cascade.mode != Layout::CASCADE_SLAVE) return;

    data_obj["cascade_num"] = layout->cascade.num;
    data_obj["sdi_format" ] = get_json_module_sdi_format();

    json["cascade_status"] = data_obj;

    QJsonDocument saveDoc(json);
    QByteArray arr = saveDoc.toJson();

    layout->cascade_server->sendClient(arr);
}
/*---------------------------------------------------------------------------*/
void Mtv_web::slot_set_cascade_data(int index)
{
QJsonObject json;
QJsonObject data_obj;

    data_obj["sdi_input_label"  ] = get_json_sdi_label();
    data_obj["layout"           ] = get_json_layout();
    data_obj["cascade_num"      ] = index + 1;
    data_obj["solo"             ] = get_json_solo();
    data_obj["sdi_out_format"   ] = layout->output_format;
    data_obj["last_slave_device"] = layout->get_last_slave_device(index);
    data_obj["gpio_mode"        ] = layout->gpio_mode;

    json["cascade_settings"] = data_obj;

    QJsonDocument saveDoc(json);
    QByteArray arr = saveDoc.toJson();

    layout->cascade_ctrl[index]->write(arr);
}
/*---------------------------------------------------------------------------*/
void Mtv_web::slot_web_new_client( QWebSocket *pClient )
{
QByteArray to_send_data;

    get_network_setting();

    to_send_data = get_json_settings();    
    web_server->senddata( pClient, to_send_data );

    to_send_data = get_json_block_configuration();
    web_server->senddata( pClient, to_send_data );
}
/*---------------------------------------------------------------------------*/
QJsonObject Mtv_web::get_json_alarm_video_settings()
{
QJsonObject alarm_video_settings_object;

    alarm_video_settings_object["threshold"] = layout->alarm_settings.video_alarm_settings.threshold;

return alarm_video_settings_object;
}
/*---------------------------------------------------------------------------*/
QJsonObject Mtv_web::get_json_alarm_audio_settings()
{
QJsonObject alarm_audio_settings_object;

    alarm_audio_settings_object["threshold"    ] = layout->alarm_settings.audio_alarm_settings.threshold;
    alarm_audio_settings_object["minimum_level"] = layout->alarm_settings.audio_alarm_settings.minimum_level;

return alarm_audio_settings_object;
}
/*---------------------------------------------------------------------------*/
QJsonObject Mtv_web::get_json_alarm_settings()
{
QJsonObject alarm_settings_object;

    alarm_settings_object["audio"] = get_json_alarm_audio_settings();
    alarm_settings_object["video"] = get_json_alarm_video_settings();

return alarm_settings_object;
}
/*---------------------------------------------------------------------------*/
QJsonObject Mtv_web::get_json_solo()
{
QJsonObject solo_object;

    solo_object["enable"] = layout->solo_mode.enable;
    solo_object["input" ] = layout->solo_mode.input;

return solo_object;
}
/*---------------------------------------------------------------------------*/
QJsonArray Mtv_web::get_json_layout_preset_name()
{
QJsonArray layout_preset_arr;

    for(int i = 0; i < layout->layout_preset.name.size(); ++i){
        layout_preset_arr.append(layout->layout_preset.name[i]);
    }

    return layout_preset_arr;
}
/*---------------------------------------------------------------------------*/
QJsonArray Mtv_web::get_json_sdi_format()
{
QJsonArray sdi_input_arr;

    for(uint i = 0; i < SizeOfArray(layout->layout_object) ; ++i){
        sdi_input_arr.append(layout->layout_object[i].sdi_format_str);
    }
    
    return sdi_input_arr;
}
/*---------------------------------------------------------------------------*/
QJsonArray Mtv_web::get_json_sdi_label()
{
QJsonArray sdi_label_arr;

    for(uint i = 0; i < SizeOfArray(layout->layout_object); ++i){
        sdi_label_arr.append(layout->layout_object[i].sdi_label);
    }

    return sdi_label_arr;
}
/*---------------------------------------------------------------------------*/
QJsonObject Mtv_web::get_json_network_setting()
{
QJsonObject eth;

    eth["ip"     ] = network_0.ip  ;
    eth["mask"   ] = network_0.mask;
    eth["gateway"] = network_0.gw  ;
    eth["mac"    ] = network_0.mac ;

    return eth;
}/*---------------------------------------------------------------------------*/
QByteArray Mtv_web::get_json_settings()
{
QJsonObject json;
QJsonObject data_obj;

    json    ["type"   ] = "settings";

    data_obj["sdi_label"       ] = get_json_sdi_label();
    data_obj["solo"            ] = get_json_solo();
    data_obj["layout"          ] = get_json_layout();
    data_obj["preset_name"     ] = get_json_layout_preset_name();
    data_obj["alarm_settings"  ] = get_json_alarm_settings();
    data_obj["network_settings"] = get_json_network_setting();
    data_obj["dns"             ] = dns_name;
    data_obj["ntp"             ] = ntp_server;
    data_obj["time_zone"       ] = time_zone;
    data_obj["model"           ] = model_device;
    data_obj["output_format"   ] = layout->output_format;
    data_obj["hdmi_color"      ] = layout->hdmi_color;
    data_obj["cascade_mode"    ] = layout->cascade.mode;
    data_obj["gpio_mode"       ] = layout->gpio_mode;
    for(uint i = 0; i < SizeOfArray(layout->cascade.ip); i++){
        QString ip_str   = "ip_" + QString::number(i + 2);
        data_obj[ip_str] = layout->cascade.ip[i];
    }

    json["data"] = data_obj;
    QJsonDocument saveDoc(json);

    return saveDoc.toJson();
}
/*---------------------------------------------------------------------------*/
QJsonObject Mtv_web::get_json_layout()
{
QJsonObject layout_object;

    layout_object["grid"              ] = layout->grid;
    layout_object["preset_index"      ] = layout->layout_preset.index;
    layout_object["clock"             ] = get_json_layout_clock();
    layout_object["time_counter"      ] = get_json_layout_time_counter();
    layout_object["teletext_enable"   ] = layout->teletext_cell.enable;
    layout_object["teletext_page"     ] = layout->teletext_cell.page;
    layout_object["video"             ] = get_json_layout_video_list();
    layout_object["labels"            ] = get_json_labels_list();
    layout_object["audio_alarm_enable"] = get_json_layout_audio_alarm();
    layout_object["video_alarm_enable"] = get_json_layout_video_alarm();
    layout_object["scte_104_display"  ] = get_json_scte_104_display();
    layout_object["aspect_ratio_sd"   ] = get_json_aspect_ratio_sd();
    layout_object["cell_style"        ] = get_json_cell_style();
    layout_object["sdi_format_display"] = get_json_sdi_format_display();
    layout_object["umd_display"       ] = get_json_umd_display();

    return layout_object;
}
/*---------------------------------------------------------------------------*/
QByteArray Mtv_web::get_json_block_configuration()
{
QJsonObject json;
QJsonObject data_obj;

    json    ["type"   ] = "configuration";

    data_obj["system"      ] = get_json_system();
    data_obj["sdi_format"  ] = get_json_sdi_format();
    data_obj["diagnostics" ] = get_json_hardware_diagnostics();
    data_obj["rand_value"  ] = rand_value;

    json["data"] = data_obj;
    QJsonDocument saveDoc(json);

    return saveDoc.toJson();
}
/*---------------------------------------------------------------------------*/
QJsonArray Mtv_web::get_json_labels_list()
{
QJsonArray item_list;

    for(uint i = 0; i < SizeOfArray(layout->layout_object); ++i){
        item_list.append(get_json_labels_object(i));
    }

    return item_list;
}
/*---------------------------------------------------------------------------*/
QJsonArray Mtv_web::get_json_layout_video_list()
{
QJsonArray item_list;

    for(uint i = 0; i < SizeOfArray(layout->layout_object); ++i){
        item_list.append(get_json_layout_video_object(i));
    }

    return item_list;
}
/*---------------------------------------------------------------------------*/
QJsonArray Mtv_web::get_json_umd_display()
{
QJsonArray item_list;

    for(uint i = 0; i < SizeOfArray(layout->layout_object); ++i){
        item_list.append(layout->layout_object[i].cell.umd_display);
    }

    return item_list;
}
/*---------------------------------------------------------------------------*/
QJsonArray Mtv_web::get_json_sdi_format_display()
{
QJsonArray item_list;

    for(uint i = 0; i < SizeOfArray(layout->layout_object); ++i){
        item_list.append(layout->layout_object[i].cell.sdi_format_display);
    }
    
    return item_list;
}
/*---------------------------------------------------------------------------*/
QJsonArray Mtv_web::get_json_cell_style()
{
QJsonArray item_list;

    for(uint i = 0; i < SizeOfArray(layout->layout_object); ++i){
        item_list.append(layout->layout_object[i].cell.style);
    }

    return item_list;
}
/*---------------------------------------------------------------------------*/
QJsonArray Mtv_web::get_json_aspect_ratio_sd()
{
QJsonArray item_list;

    for(uint i = 0; i < SizeOfArray(layout->layout_object); ++i){
        item_list.append(layout->layout_object[i].cell.aspect_ratio_sd);
    }

    return item_list;
}
/*---------------------------------------------------------------------------*/
QJsonArray Mtv_web::get_json_scte_104_display()
{
QJsonArray item_list;

    for(uint i = 0; i < SizeOfArray(layout->layout_object); ++i){
        item_list.append(layout->layout_object[i].cell.scte_104_display);
    }

    return item_list;
}
/*---------------------------------------------------------------------------*/
QJsonArray Mtv_web::get_json_layout_video_alarm()
{
QJsonArray item_list;

    for(uint i = 0; i < SizeOfArray(layout->layout_object); ++i){
        item_list.append(layout->layout_object[i].cell.video_alarm.enable);
    }

    return item_list;
}
/*---------------------------------------------------------------------------*/
QJsonArray Mtv_web::get_json_layout_audio_alarm()
{
QJsonArray item_list;

    for(uint i = 0; i < SizeOfArray(layout->layout_object); ++i){
        item_list.append(layout->layout_object[i].cell.audio_alarm.enable);
    }

    return item_list;
}
/*---------------------------------------------------------------------------*/
QJsonObject Mtv_web::get_json_layout_time_counter()
{
QJsonObject layout_time_counter_object;

    layout_time_counter_object["enable" ] = layout->time_counter_cell.enable;
    layout_time_counter_object["scale_x"] = layout->time_counter_cell.scale_x;
    layout_time_counter_object["scale_y"] = layout->time_counter_cell.scale_y;
    layout_time_counter_object["x"      ] = layout->time_counter_cell.x;
    layout_time_counter_object["y"      ] = layout->time_counter_cell.y;

    return layout_time_counter_object;
}
/*---------------------------------------------------------------------------*/
QJsonObject Mtv_web::get_json_layout_clock()
{
QJsonObject layout_clock_object;

    layout_clock_object["scale_x"    ] = layout->clock_cell.scale_x;
    layout_clock_object["scale_y"    ] = layout->clock_cell.scale_y;
    layout_clock_object["enable"     ] = layout->clock_cell.enable;
    layout_clock_object["label"      ] = layout->clock_cell.label;
    layout_clock_object["x"          ] = layout->clock_cell.x;
    layout_clock_object["y"          ] = layout->clock_cell.y;
    layout_clock_object["style"      ] = layout->clock_cell.style;
    layout_clock_object["date_locale"] = layout->clock_cell.date_locale;

    return layout_clock_object;
}
/*---------------------------------------------------------------------------*/
QJsonObject Mtv_web::get_json_labels_object(int i)
{
QJsonObject layout_object;

    layout_object["scale_x"] = layout->label_cell[i].scale_x;
    layout_object["scale_y"] = layout->label_cell[i].scale_y;
    layout_object["enable" ] = layout->label_cell[i].enable;
    layout_object["x"      ] = layout->label_cell[i].x;
    layout_object["y"      ] = layout->label_cell[i].y;
    layout_object["text"   ] = layout->label_cell[i].label.text;

    return layout_object;
}
/*---------------------------------------------------------------------------*/
QJsonObject Mtv_web::get_json_layout_video_object(int i)
{
QJsonObject layout_object;

    layout_object["scale_x"] = layout->layout_object[i].cell.scale_x;
    layout_object["scale_y"] = layout->layout_object[i].cell.scale_y;
    layout_object["enable" ] = layout->layout_object[i].cell.enable;
    layout_object["x"      ] = layout->layout_object[i].cell.x;
    layout_object["y"      ] = layout->layout_object[i].cell.y;
    layout_object["bars"   ] = layout->layout_object[i].cell.audio_meter;
    layout_object["audio_alarm_channel_enable"] = get_json_alarm_channel_enable(i);

    return layout_object;
}
/*---------------------------------------------------------------------------*/
QJsonArray Mtv_web::get_json_alarm_channel_enable(int index)
{
QJsonArray item_list;

    for(uint i = 0; i < SizeOfArray(layout->layout_object[0].cell.audio_alarm_channel_enable); ++i){
        item_list.append(layout->layout_object[index].cell.audio_alarm_channel_enable[i]);
    }

    return item_list;
}
/*---------------------------------------------------------------------------*/
QJsonObject Mtv_web::get_json_hardware_diagnostics()
{
    QJsonObject options_obj;
    QDateTime current_datetime = QDateTime::currentDateTime();

    options_obj["temperature_C"] = temperature_C;
    options_obj["power_W"      ] = power_W;
    options_obj["fan"          ] = fan_state;
    options_obj["uptime"       ] = uptime_utils->get_sys_uptime();
    options_obj["sys_date_time"] = (int32_t) current_datetime.toTime_t();

    return options_obj;
}
/*---------------------------------------------------------------------------*/
QJsonObject Mtv_web::get_json_system()
{
    QJsonObject sys_obj;
    #ifdef APP_VERSION   // сборка из под Yocto
        sys_obj["version_software"   ] = QString(APP_VERSION);
    #else               // сборка из под SDK
        sys_obj["version_software"   ] = VERSION;
    #endif
    sys_obj["build_id"           ] = mtvsystem->get_build_id();

    return sys_obj;
}
/*---------------------------------------------------------------------------*/
void Mtv_web::Settings_Read(){

    QSettings settings(QSettings::SystemScope, SETTINGS_SYS_CONFIG_FILE_NAME);
    settings.beginGroup("time");
        ntp_server = settings.value("npt_server", "pool.ntp.org" ).toString();
        time_zone   = settings.value("timezone",  "Europe/Moscow").toString();
    settings.endGroup();
    settings.beginGroup("DNS");
        dns_name = settings.value("dns_name", "8.8.4.4").toString();
    settings.endGroup();
    set_sys_conf();
    qDebug(category) << "\n\t\tntp_server : " << ntp_server
                    << "\n\t\tdns_name : " << dns_name
                    << "\n\t\ttime_zone : " << time_zone;
} // End "Settings_Read"

void Mtv_web::Check(){
    qDebug() << "void check";
}
/*---------------------------------------------------------------------------*/
void Mtv_web::Settings_Write(){
    QSettings settings(QSettings::SystemScope, SETTINGS_SYS_CONFIG_FILE_NAME);
    settings.beginGroup("time");
        settings.setValue("npt_server", ntp_server);
        settings.setValue("timezone",   time_zone);
    settings.endGroup();

    settings.beginGroup("DNS");
        settings.setValue("dns_name", dns_name);
    settings.endGroup();
    qDebug(category) << " 1191 Settings_Write()";
} // End "Settings_Write"
/*---------------------------------------------------------------------------*/
void Mtv_web::get_network_setting()
{
char ip[18],  mask[18], gw[18];
static const char *eth0="eth0";


    // eth0 settings
    memset(ip,   0, sizeof(ip));
    memset(mask, 0, sizeof(mask));
    memset(gw,   0, sizeof(gw));
    ip_get_ip(eth0, ip, mask, gw);

    network_0.ip   = ip;
    network_0.mask = mask;
    network_0.gw   = gw;

    // get Hardware Address
    foreach(QNetworkInterface netInterface, QNetworkInterface::allInterfaces())
    {
        if(!(netInterface.flags() & QNetworkInterface::IsLoopBack))
        {
            if(netInterface.name() == "eth0")   {                
                network_0.mac = netInterface.hardwareAddress();
                // ign added get ip
                if (netInterface.hardwareAddress().toUpper() == network_0.mac.toUpper()) {
                    QList<QHostAddress> entries = netInterface.allAddresses();
                    for (const QHostAddress &entry : entries) {
                        if(!entry.isLoopback() && entry.protocol() == QAbstractSocket::IPv4Protocol){
                            qDebug(category) << "\tconnected IP:" << entry.toString() << "MAC" << network_0.mac;
                        }
                    }
                }    
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
QByteArray Mtv_web::get_json_layout_presets()
{
QJsonObject json;
QJsonObject data_obj;
QJsonObject preset_obj;
QJsonArray preset_files;

    for(int i = 0; i < 10; ++i){

        QString s = read_preset_file(i);
        preset_files.append(s);
    }

    preset_obj["preset_name" ] = get_json_layout_preset_name();
    preset_obj["preset_files"] = preset_files;

    json["type"] = "layout_presets";
    data_obj["preset_obj"] = preset_obj;
    json["data"] = data_obj;
    QJsonDocument saveDoc(json);

    return saveDoc.toJson();
}
/*---------------------------------------------------------------------------*/
QString  Mtv_web::get_preset_file_name(int file_num)
{
    QString file_name = layout->get_preset_file_name(file_num);

    QSettings settings(QSettings::SystemScope, file_name);

    return settings.fileName();
}
/*---------------------------------------------------------------------------*/
QString  Mtv_web::read_preset_file(int file_num)
{
    QString file_name = get_preset_file_name(file_num);

    QFile file(file_name);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return "";

    QTextStream in(&file);

    QString text = in.readAll();

    file.close();

    return text;
}
/*---------------------------------------------------------------------------*/
void Mtv_web::cmd_set_layout_presets(QJsonObject data_obj)
{
QJsonObject preset_obj;
QJsonArray preset_files_arr;
QJsonArray preset_name_arr;

    get_Json_object(data_obj, "preset_obj", preset_obj);

    get_Json_Array(preset_obj, "preset_name", preset_name_arr);
    parser_preset_name(preset_name_arr);

    get_Json_Array(preset_obj, "preset_files", preset_files_arr);
    parser_preset_files(preset_files_arr);

    update_preset();
}
/*---------------------------------------------------------------------------*/
void Mtv_web::parser_preset_files(QJsonArray preset_files_arr)
{
QString s;

    for(int i = 0; i < preset_files_arr.size(); i++){

        s = preset_files_arr[i].toString();

        if(s.size() > 0)
            write_preset_file(i, preset_files_arr[i].toString());
    }
}
/*---------------------------------------------------------------------------*/
void  Mtv_web::write_preset_file(int file_num, QString text)
{
    QString file_name = get_preset_file_name(file_num);

    QFile file_to_write(file_name);
       if (!file_to_write.open(QIODevice::WriteOnly | QIODevice::Text))
           return;

    QTextStream out(&file_to_write);
    out << text;
}
/*---------------------------------------------------------------------------*/
void Mtv_web::parser_preset_name(QJsonArray preset_name_arr)
{
    int layout_object_size = layout->layout_preset.name.size();
    for(int i = 0; i < preset_name_arr.size(); i++){
        if(i >= layout_object_size)
            break;
        layout->layout_preset.name[i] =  preset_name_arr[i].toString();
    }
}
/*---------------------------------------------------------------------------*/
