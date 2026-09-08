#ifndef LAYOUT_H
#define LAYOUT_H

#include <QObject>
#include <QDebug>
#include <QImage>
#include <QPainter>
#include <QStaticText>
#include <QSettings>
#include <QWidget>
#include <QTime>
#include <QElapsedTimer>
#include <QFontMetrics>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSvgRenderer>

#include "../mtv-system/mtv-system.h"
//#include "../m26/mb86m26_control.h"
#include "../led/led_class.h"
#include "../qpps/qpps.h"
#include "../gpio/gpio.h"
#include "../eventlog-api/eventlog.h"
#include "../cascade_server/cascade_server.h"
#include "../cascade_ctrl/cascade_ctrl.h"
#include "../time_counter/time_counter.h"

#define CASCADE_NUM 4 // кол-во блоков в каскаде

class Layout : public QObject
{
    Q_OBJECT
public:
    //explicit Layout(PbxMtvSystem *mtvsystem,  mb86m26_control *m26_control, Gpio *gpio, Eventlog *eventlog);
    explicit Layout(PbxMtvSystem *mtvsystem,  Gpio *gpio, Eventlog *eventlog);
    ~Layout();

    typedef struct {
        QPoint pos;
        int enable;
    } audio_meters_t;

    typedef struct {
        int enable;
        int input;
    } solo_mode_t;

    enum cell_type_t{NONE, VIDEO, CLOCK, LABEL, LOGO};
    enum grid_type_t{GRID_3x3, GRID_4x4, GRID_5x5, GRID_6x6, GRID_12x12};
    enum alarm_type{VIDEO_LOST, AUDIO_SILENCE, VIDEO_FREEZE};
    enum output_format{OUT_STD_1080p25, OUT_STD_1080i50};
    enum cascade_mode_t{STAND_ALONE, CASCADE_1, CASCADE_2, CASCADE_3, CASCADE_4, CASCADE_SLAVE};
    enum label_display_mode_t{STATIC_ONLY, UMD_ONLY, STATIC_AND_UMD};
    enum tally_indicator_mode_t{TALLY_OFF, TALLY_RED, TALLY_GREEN, TALLY_RED_AND_GREEN};

    typedef struct{
        int             enable;
        QElapsedTimer   time_threshold_ok;
        QElapsedTimer   time_threshold_error;
    } alert_t;

    typedef struct{
        int x;                  // координаты в ячейках (0..3)
        int y;                  // координаты
        int scale_x;
        int scale_y;
        int enable;
        int style;              // Часы: 0 - аналоговые; 1 - цифровые.
        int date_locale;        // язык отображения даты в цифровых часах
        QString label;
    } clock_cell_t;

    typedef struct{
        int enable;
        int page;
        QRect frame;
        QRect panel_teletext;
    } teletext_cell_t;

    typedef struct{
        int x;                  // координаты в ячейках (GRID_3x3 .. GRID_12x12)
        int y;                  // координаты в ячейках (GRID_3x3 .. GRID_12x12)
        int scale_x;
        int scale_y;
        int enable;
        QRect frame;
    } time_counter_cell_t;

    typedef struct{
        QRect cell;
        QRect size;
        QString text;
        QFont   font;
    } label_t;

    typedef struct{
        int x;                  // координаты в ячейках (GRID_3x3 .. GRID_12x12)
        int y;                  // координаты в ячейках (GRID_3x3 .. GRID_12x12)
        int scale_x;
        int scale_y;
        int enable;
        label_t label;
        QRect frame;
    } label_cell_t;

    typedef struct{
        int x;                  // координаты в ячейках (0..3)
        int y;                  // координаты
        int scale_x;
        int scale_y;
        int enable;
        int sdi_format_display;    // отображать формат сигнала
        int teletext_icon_display; // отображать иконку телетекста
        int umd_display;           // отображать UMD (Under monitor displays)
        int enable_label;          // отображать имя входа
        int input;                 // номер входа, с которого идет сигнал (когда будет реализован повтор)
        int border_color;          // цвет рамки
        int audio_meter;           // количество индикаторов уровня
        int audio_alarm_channel_enable[4];
        int scte_104_display;
        int aspect_ratio_sd;    // 0 - "4:3", 1 - "16:9";
        int style;              // 0 - default(A), 1 - retro style(B);
        alert_t audio_alarm;
        alert_t video_alarm;
    } cell_t;

    typedef struct{
        QString         text;
        QColor          bkground_color;
        QElapsedTimer   time;
        int             type;
    } alarm_t;


    typedef struct{
        QRect plane_video;
        label_t panel_label;
        QRect panel_tally_red;
        QRect panel_tally_green;
    } screen_old_style_t;

    typedef struct{
        QRect cell;
        QRect plane_video;
        QRect panel_label;
        QRect panel_format_video;
        QRect panel_text_icons;
        QRect panel_scte_104;
        int   enable_video;
    } screen_plan_t;

    typedef struct{
        QRect cell;
        QRect clock_size;
        QRect label_rec;
        QRect date_rec;
        QString text;
    } analog_clock_t;

    typedef struct{
        cell_t              cell;
        QList<alarm_t>      alarm;
        screen_plan_t       screen_plan;
        screen_old_style_t  screen_old_style;
        QString             sdi_label;
        QString             sdi_format_str;
        int                 tally;
    } layout_object_t;

    typedef struct{
        int             index;
        QList<QString>   name;
    } layout_preset_t;


    typedef struct{
        int             threshold;
        int             minimum_level;
    } audio_alarm_settings_t;

    typedef struct{
        int             threshold;
    } video_alarm_settings_t;

    typedef struct{
        audio_alarm_settings_t audio_alarm_settings;
        video_alarm_settings_t video_alarm_settings;
    } alarm_settings_t;

    typedef struct{
        QString in;
        QString out;
    } scte_104_splice_t;

    typedef struct{
        int     num;                //  номер устройства в каскад
        int     mode;               //  0 - Standalone; 1 - Cascade_1; 2 - Cascade_2; 3 - Cascade_3; 4 - Cascade_4
        int     last_slave_device;  //  последний в цепочке каскадирования
        QString ip[CASCADE_NUM];
    } cascade_t;

    Cascade_ctrl    *cascade_ctrl[CASCADE_NUM];

    solo_mode_t solo_mode;
    scte_104_splice_t   scte_104_splice[16];
    layout_object_t     layout_object[(CASCADE_NUM + 1) * 16];
    label_cell_t        label_cell[(CASCADE_NUM + 1) * 16];
    clock_cell_t        clock_cell;
    teletext_cell_t     teletext_cell;
    time_counter_cell_t time_counter_cell;
    label_t             time_counter;
    analog_clock_t      analog_clock;
    analog_clock_t      digital_clock;
    layout_preset_t     layout_preset;
    alarm_settings_t    alarm_settings;
    cascade_t           cascade;
    int                 gpio_mode;
    int                 output_format;
    int                 hdmi_color;
    int grid;               // размер сетки (3x3, 4x4, 5x5, 6x6)
    int width_grid, height_grid;
    int op47[16];
    int op47_latch[16];

    void cell(QImage &image, QRect boundary, QColor color, QString text);

    void draw_overlay();
    void draw_overlay_test_file();
    void routing_source_video();
    void Settings_Read();
    void Settings_Write();
    void Clear_SDI_input_Settings_File();
    void update_layout();
    void preset_Layout_Read (int preset_num);
    void preset_Layout_Write(int preset_num);
    void draw_alarm_label(int index, layout_object_t layout_objectl);
    void clear_alarm(layout_object_t layout_object);
    void display_teletext(QImage image_teletext);
    void cascade_udate();
    void query_cascade_data();
    int  get_last_slave_device(int index);
    void eventlog_cascade(int category, QString message);
    void eventlog_add_input_state(QString pStr, int num_input);
    void update_label_name_channel(int index, QString txt);
    void DisplayTALLY(int addr, int tally);
    QString get_num_input(int in_num);
    QString get_preset_file_name(int preset_num);

    Cascade_server  *cascade_server;
    Time_counter    *timer_time_counter;

private:
    QImage *image_clock;
    QTimer timer_update_alarm;
    QTimer timer_update_op47;
    QTimer timer_analog_clock;      // для плавного хода секундной стрелки (чаще 1 раза в секунду)

    QSvgRenderer m_clock_face;
    QImage       m_clock_face_cache;   // циферблат не меняется - рендерим SVG один раз, дальше просто копируем

    PbxMtvSystem    *mtvsystem;
    //mb86m26_control *m26_control;
    Led_class       *led;
    QPps            *qpps;
    Gpio            *gpio;
    Eventlog        *eventlog;
    QImage  image_sound;
    QImage  image_teletext;
    QImage get_layout_1x1(int index);
    QImage get_layout();
    void get_grid_size();
    void set_cell_plane_old_style(layout_object_t &cell_object);
    void disable_all_audio_meters();
    void draw_grid_1x1(QImage &image);
    void draw_transparant_text_panel(QImage &image, QRect panel, int FontSize, int AlignmentFlag, QString text);
    void draw_old_style_text_panel(QImage &image,  QRect panel, QFont font, QString text);
    int  draw_text_icon(QImage &image,  QRect panel, QString text);
    void set_aspect_ratio(QRect &rec, int index);
    void set_audio_meter_position(int index);
    void set_cell_plane(layout_object_t &cell_object);
    void draw_frame(QImage &image, QColor color, QRect boundary, int width);
    void draw_TALLY_frame(QImage &image, QColor color, QRect boundary);
    void pip_config(int i);
    void draw_layout_object(QImage &image, layout_object_t layout_object);
    void draw_label_object(QImage &image, label_cell_t label_cell);
    void set_layout_plane();
    void set_label_plane();
    void set_info_panel_size(layout_object_t &cell);
    void draw_analog_clock();
    void draw_digital_clock();
    void draw_time_counter(label_t label);
    void set_clock_plane();
    void set_time_counter_plane();
    void get_ratio_4_3(QRect &pip);
    void get_ratio_492_250(QRect &pip);
    void set_teletext_plane(QRect cell);
    void draw_layout_analog_clock(QImage &image);
    void draw_layout_digital_clock(QImage &image);
    void draw_time_counter_frame(QImage &image);
    void draw_teletext_frame(QImage &image);
    void update_sdi_format(int index);
    void update_alarm();
    QRect get_alarm_label_rec(QRect cell);
    QRect get_TALLY_indicator_rec(QRect cell);
    void draw_TALLY_indicator(int tally_indicator_mode, const layout_object_t &layout_object);
    void get_image_TALLY_indicator_old_style(QImage &image, const QRect &cell, const QColor &color);
    void draw_TALLY_indicator_old_style(QRect rec, QColor color);
    void draw_alarm_elapsed();
    QString sec_to_TimeStr(qint64 sec_val);
    int  get_alarm_index(layout_object_t layout_object, int alarm_type);
    void check_audio(int cell_index, QList<int> level_list);
    void ini_alarm_time_threshold();
    void check_freeze(int cell_index);
    void check_video_loss(int cell_index);
    int  get_common_alarm();
    void display_op47_icons(int cell_index);
    void display_text_icons(QImage &image, QRect panel, int cell_index);
    void display_scte_104(int index);
    void clear_TALLY_indicator(layout_object_t layout_object);
    void draw_TALLY_indicator_old_style(int state, const layout_object_t &layout_object);
    void scte_104_update();
    void clean_teletext_image(int channel);
    void cascade_mode_update();
    int  enable_cascade(int num);
    void debugPrintJson(QString str, QByteArray data);
    QString sdi_key_name(int i);
    void sound_routing();
    void emit_signal_solo(solo_mode_t solo_mode);
    QImage fast_scale(QImage image, int width, int height);

    QImage full_overlay_frame;   // общий кадр 1920x1080, накапливает все элементы
    void blit_to_frame(QImage *image, int x, int y);
    void flush_overlay();

signals:
    void signal_solo(solo_mode_t solo_mode);
    void signal_preset(int preset_number);
    void signal_common_alarm_changed(int common_alarm);
    void signal_cascade_device_connected(int index);
    void signal_cascade_device_data_receive(int index, QByteArray data);
    void signal_cascade_server_readyRead(QByteArray arr);

public slots:
    void slot_new_format();
    void slot_qpps();
    void slot_update_alarm();
    void slot_update_op47();
    void slot_gpio_solo(int input_number);
    void slot_gpio_preset(int preset_number);
    void slot_cascade_gpio_set_solo(solo_mode_t solo_mode);
    void slot_solo_mode_desebled();
    void slot_op47_data(int channel, QByteArray data);
    void slot_splice(int index, QString text_in, QString text_out);
    void slot_master_connected(QTcpSocket* pSocket, QString ip);
    void slot_Disconnected(QTcpSocket* pSocket);
    void slot_tcp_server_readyRead(QTcpSocket* pSocket, QByteArray data);
    void slot_cascade_device_connected(int index);
    void slot_cascade_device_data_receive(int index, QByteArray data);
    void slot_TALLY(int input, int state);
    void slot_draw_time_counter(QString time_counter_str);
    void slot_draw_analog_clock_tick();
};

#endif // LAYOUT_H