#include "layout.h"
#include <unistd.h>
#include <QLoggingCategory>
#include <QSvgRenderer>
#include <QLocale>
#include <QPainterPath> // ign

#define LAYOUT_PRESET_FILE_NAME      ("pbx-mtv-508_layout_preset")
#define SETTINGS_SDI_INPUT_FILE_NAME ("pbx-mtv-508_sdi_input")
#define TCP_PORT 10110

static QLoggingCategory category("\033[32m Layout Class \033[0m");
#define PEN_WIDTH 2
#define SizeOfArray(a) (sizeof(a)/sizeof(*a))
#define FONT_SDI_FORMAT_SIZE 14
#define FONT_CHANNEL_NAME_SIZE 19

#define  WIDTH_3x3 (1920 / 3)
#define HEIGHT_3x3 (1080 / 3)

#define  WIDTH_4x4 (1920 / 4)
#define HEIGHT_4x4 (1080 / 4)

#define  WIDTH_5x5 (1920 / 5)
#define HEIGHT_5x5 (1080 / 5)

#define  WIDTH_6x6 (1920 / 6)
#define HEIGHT_6x6 (1080 / 6)

#define  WIDTH_12x12 (1920 / 12)
#define HEIGHT_12x12 (1080 / 12)

#define TALLY_RED_OFF_COLOR QColor(0x00, 0x00, 0x00)
#define TALLY_GREEN_OFF_COLOR QColor(0x00, 0x00, 0x00)



//Layout::Layout(PbxMtvSystem *mtvsystem,  mb86m26_control *m26_control, Gpio *gpio, Eventlog *eventlog) :
//    mtvsystem(mtvsystem), m26_control(m26_control), gpio(gpio), eventlog(eventlog)
Layout::Layout(PbxMtvSystem *mtvsystem,  Gpio *gpio, Eventlog *eventlog) :
    mtvsystem(mtvsystem), gpio(gpio), eventlog(eventlog)
{
    qCDebug(category) << "creating...";

    for(int i = 0; i < 16; i++) op47[i] = 0;
    for(int i = 0; i < 16; i++) op47_latch[i] = 0;
    output_format = OUT_STD_1080i50;

    for(uint i = 0; i < SizeOfArray(layout_object); ++i) layout_object[i].tally = 0;

    led = new Led_class;

    connect(mtvsystem, &PbxMtvSystem::signal_new_format, this, &Layout::slot_new_format);

    qpps = new QPps;
    connect(qpps, &QPps::pps, this, &Layout::slot_qpps);

    timer_update_alarm.start(250);
    connect(&timer_update_alarm, &QTimer::timeout, this, &Layout::slot_update_alarm);

    timer_update_op47.start(5000);
    connect(&timer_update_op47, &QTimer::timeout, this, &Layout::slot_update_op47);

    connect(gpio, &Gpio::signal_solo,               this, &Layout::slot_gpio_solo);
    connect(gpio, &Gpio::signal_preset,             this, &Layout::slot_gpio_preset);
    connect(gpio, &Gpio::signal_solo_mode_desebled, this, &Layout::slot_solo_mode_desebled);
    connect(gpio, &Gpio::signal_TALLY,              this, &Layout::slot_TALLY);

    timer_time_counter = new Time_counter;
    connect(timer_time_counter, &Time_counter::signal_time_counter_update, this, &Layout::slot_draw_time_counter);
    connect(gpio, &Gpio::signal_time_count_start, timer_time_counter, &Time_counter::slot_start);
    connect(gpio, &Gpio::signal_time_count_stop,  timer_time_counter, &Time_counter::slot_stop);
    emit gpio->signal_time_count_start();

    // SVG циферблата грузим один раз; сами стрелки теперь рисуются вручную (см. draw_analog_clock)
    m_clock_face.load(QString(":/image/clock/face.svg"));

    timer_analog_clock.start(50); // 10 (20) Гц — достаточно, чтобы секундная стрелка выглядела "скользящей"
    connect(&timer_analog_clock, &QTimer::timeout, this, &Layout::slot_draw_analog_clock_tick);

    ini_alarm_time_threshold();
    hdmi_color = 1;

    Settings_Read();

    gpio->set_mode(gpio_mode);
    preset_Layout_Read(layout_preset.index);
    qDebug(category) << "\t\tpreset_Layout_Read" << layout_preset.name[0] << "\033[0m";

    sound_routing();
    QImage my_image; //ign
    //get_layout();
    my_image = get_layout(); // ign
    
    qDebug(category) <<"\t\tget_layout returned my_image" << my_image.size();

    cascade_server = new Cascade_server(TCP_PORT);
    connect(cascade_server, &Cascade_server::signal_new_client,    this, &Layout::slot_master_connected);
    connect(cascade_server, &Cascade_server::signal_pDisconnected, this, &Layout::slot_Disconnected);
    connect(cascade_server, &Cascade_server::signal_readyRead,     this, &Layout::slot_tcp_server_readyRead);

    for(uint i = 0; i < SizeOfArray(cascade.ip); i++){
        int enable = enable_cascade(i);
        cascade_ctrl[i] = new Cascade_ctrl(i, cascade.ip[i], TCP_PORT, enable);
        connect(cascade_ctrl[i], &Cascade_ctrl::signal_connected,    this,  &Layout::slot_cascade_device_connected);
        connect(cascade_ctrl[i], &Cascade_ctrl::signal_data_receive, this,  &Layout::slot_cascade_device_data_receive);
    }

    image_teletext.load(":/image/teletext.png");
    image_teletext = image_teletext.scaled(QSize(46, 46));

    time_counter.text = "00:00:00";

    full_overlay_frame = QImage(1920, 1080, QImage::Format_ARGB32);
    full_overlay_frame.fill(0x00000000); // изначально всё прозрачно

}

Layout::~Layout()
{
    delete led;
    delete qpps;
    delete cascade_server;
    delete timer_time_counter;
    for(uint i = 0; i < SizeOfArray(cascade_ctrl); i++)
        delete cascade_ctrl[i];
}

void Layout::query_cascade_data()
{
    cascade_server->closeAllConnections(); // Закрыть все tcp-соединения. При новом подключении будут высланы новые данный
}

int Layout::enable_cascade(int num)
{
    if(cascade.mode == STAND_ALONE  ) return 0;
    if(cascade.mode >= CASCADE_SLAVE) return 0;
    if(num < cascade.mode)
        return 1;
    else
        return 0;
}

void Layout::debugPrintJson(QString str, QByteArray data)
{
    QJsonParseError err;
    QJsonDocument saveDoc = QJsonDocument::fromJson(data, & err);
    qCDebug(category) << str << "\n" << saveDoc.toJson().toStdString().c_str();
}


int Layout::get_last_slave_device(int index)
{
    if(cascade.mode == (index + 1))
        return 1;
    else
        return 0;
}

void Layout::slot_cascade_device_data_receive(int index, QByteArray data)
{
    emit signal_cascade_device_data_receive(index, data);
}

void Layout::slot_cascade_device_connected(int index)
{
    emit signal_cascade_device_connected(index);
}


void Layout::slot_master_connected(QTcpSocket* pSocket, QString ip)
{
    qCDebug(category) << "The master connected with ip:" << ip << "port:" << pSocket->localPort();
}

void Layout::slot_Disconnected(QTcpSocket* pSocket)
{
    qCDebug(category) << "disconnected:" << pSocket->peerAddress().toString();
}

void Layout::slot_tcp_server_readyRead(QTcpSocket* pSocket, QByteArray data)
{
    Q_UNUSED(pSocket);

    if(cascade.mode != cascade_mode_t::CASCADE_SLAVE) return;

    emit signal_cascade_server_readyRead(data);
}

void Layout::slot_solo_mode_desebled()
{
solo_mode_t solo_mode;

    solo_mode.enable = 0;
    solo_mode.input = this->solo_mode.input & 0x0F;

    emit_signal_solo(solo_mode);
}

void Layout::slot_gpio_preset(int preset_number)
{
    if(cascade.mode == Layout::CASCADE_SLAVE) return;
    emit signal_preset(preset_number);
}


void Layout::slot_gpio_solo(int input_number)
{
solo_mode_t solo_mode;

    if(!cascade.last_slave_device && (input_number == 15)) return ;

    solo_mode.enable = 1;
    solo_mode.input  = input_number;

    emit_signal_solo(solo_mode);
}

void Layout::emit_signal_solo(solo_mode_t solo_mode)
{
    if(cascade.mode == Layout::CASCADE_SLAVE){
        this->solo_mode.input = solo_mode.input;;
        slot_cascade_gpio_set_solo(solo_mode);
    }else{
        emit signal_solo(solo_mode);
    }
}

void Layout::slot_cascade_gpio_set_solo(solo_mode_t solo_mode)
{
QJsonObject json;
QJsonObject data_obj;

    if(cascade.mode != Layout::CASCADE_SLAVE) return;
    if(!cascade.last_slave_device && (solo_mode.input == 15) && solo_mode.enable) return;

    data_obj["enable"] = solo_mode.enable;
    data_obj["input" ] = cascade.num * 16 + solo_mode.input;

    json["set_solo"] = data_obj;

    QJsonDocument saveDoc(json);
    QByteArray arr = saveDoc.toJson();

    cascade_server->sendClient(arr);
}


void Layout::slot_qpps()
{
    if(clock_cell.style)
        draw_digital_clock();
    // аналоговые часы теперь перерисовываются отдельным таймером timer_analog_clock
    // (см. slot_draw_analog_clock_tick), чтобы секундная стрелка двигалась плавно

    draw_alarm_elapsed();

    // qDebug(category) << "slot_qpps";
    // flush_overlay();
}


void Layout::slot_update_alarm()
{
static int common_alarm_old = -1;

    QList<int>level_list = mtvsystem->get_audio_level();

    for(int i_cell = 0; i_cell < 16; i_cell++){
        check_audio(i_cell, level_list);
        check_freeze(i_cell);
        check_video_loss(i_cell);
    }

    // flush_overlay();

    int common_alarm = get_common_alarm();
    gpio->set_common_alarm(common_alarm);

    if( common_alarm_old != common_alarm){
        common_alarm_old = common_alarm;
        emit signal_common_alarm_changed(common_alarm);
    }

}

void Layout::check_freeze(int cell_index)
{
int error, error_old;

    error = 0;
    int k = cascade.num * 16 + cell_index;
    if(layout_object[k].cell.video_alarm.enable){
       error = !mtvsystem->get_motion(cell_index);

        if(!layout_object[k].cell.video_alarm.enable)
                error = 0;

        if(error)
            layout_object[k].cell.video_alarm.time_threshold_error.start();
        else
            layout_object[k].cell.video_alarm.time_threshold_ok.start();

        qint64 elapsed_error = layout_object[k].cell.video_alarm.time_threshold_error.elapsed();
        qint64 elapsed_ok    = layout_object[k].cell.video_alarm.time_threshold_ok.elapsed();
        qint64 threshold     = alarm_settings.video_alarm_settings.threshold;

        if((elapsed_error < threshold) && (elapsed_ok < threshold)) return;
    }

    if(!mtvsystem->get_sdi_status(cell_index))
        error = 0;

    int index = get_alarm_index(layout_object[k], VIDEO_FREEZE);
    if(index < 0)
        error_old = 0;
    else
        error_old = 1;

    if(error_old == error) return;

    clear_alarm(layout_object[k]);

    if(error){
        if(!(k == 0 && teletext_cell.enable))
        {
            alarm_t alarm;
            alarm.bkground_color = QColor(255, 0, 0, 225);
            alarm.text = "Video freeze";
            alarm.type = VIDEO_FREEZE;
            alarm.time.start();
            layout_object[k].alarm.append(alarm);
        }
    }
    else{
        layout_object[k].alarm.removeAt(index);
    }

    QString str, state_str;

    if(error)
        state_str = "Freeze";
    else
        state_str = "UnFreeze";

    eventlog_add_input_state("Video Input %1: " + state_str, cell_index);
}

void Layout::eventlog_add_input_state(QString pStr, int num_input)
{
    QString num_input_str = get_num_input(num_input);

    QString str = pStr.arg(num_input_str);

    eventlog->add(Eventlog::INPUT_STATE, str);
    eventlog_cascade(Eventlog::INPUT_STATE, str);
}


void Layout::eventlog_cascade(int category, QString message)
{
QJsonObject json;
QJsonObject data_obj;

    if(cascade.mode != Layout::CASCADE_SLAVE) return;
    if(!cascade.last_slave_device && (solo_mode.input == 15)) return;

    data_obj["category"] = category;
    data_obj["message" ] = message;

    json["eventlog_cascade"] = data_obj;

    QJsonDocument saveDoc(json);
    QByteArray arr = saveDoc.toJson();

    cascade_server->sendClient(arr);
}


void Layout::ini_alarm_time_threshold()
{
    for(uint i = 0; i < SizeOfArray(layout_object); i++){
        layout_object[i].cell.audio_alarm.time_threshold_ok.start();
        layout_object[i].cell.audio_alarm.time_threshold_error.start();

        layout_object[i].cell.video_alarm.time_threshold_ok.start();
        layout_object[i].cell.video_alarm.time_threshold_error.start();
    }
}

void Layout::check_audio(int cell_index, QList<int> level_list)
{
int error, error_old;

    error = 0;
    int k = cascade.num * 16 + cell_index;

    if(layout_object[k].cell.audio_alarm.enable){
        for(int i = 0; i < 4; i++){
            if(layout_object[k].cell.audio_alarm_channel_enable[i]){
                int level = level_list.at(cell_index * 4 + i);
                if(level < alarm_settings.audio_alarm_settings.minimum_level){
                    error = 1;
                    break;
                }
            }
        }

        if(error)
            layout_object[k].cell.audio_alarm.time_threshold_error.start();
        else
            layout_object[k].cell.audio_alarm.time_threshold_ok.start();

        qint64 elapsed_error = layout_object[k].cell.audio_alarm.time_threshold_error.elapsed();
        qint64 elapsed_ok    = layout_object[k].cell.audio_alarm.time_threshold_ok.elapsed();
        qint64 threshold     = alarm_settings.audio_alarm_settings.threshold;

        if((elapsed_error < threshold) && (elapsed_ok < threshold)) return;
    }

    if(!mtvsystem->get_sdi_status(cell_index))
        error = 0;


    int index = get_alarm_index(layout_object[k], AUDIO_SILENCE);
    if(index < 0)
        error_old = 0;
    else
        error_old = 1;

    if(error_old == error) return;

    clear_alarm(layout_object[k]);

    if(error){
        if(!(k == 0 && teletext_cell.enable))
        {
            alarm_t alarm;
            alarm.bkground_color = QColor(255, 0, 0, 225);
            alarm.text = "Audio silence";
            alarm.type = AUDIO_SILENCE;
            alarm.time.start();
            layout_object[k].alarm.append(alarm);
        }
    }
    else{
        layout_object[k].alarm.removeAt(index);
        }

    if(mtvsystem->get_sdi_status(cell_index)){
        QString str, state_str;

        if(error)
            state_str = "Silence";
        else
            state_str = "Ok";

        eventlog_add_input_state("Audio Input %1: " + state_str, cell_index);
    }
}

int Layout::get_alarm_index(layout_object_t layout_object, int alarm_type)
{
int index = -1;

    for(int i = 0; i < layout_object.alarm.size(); ++i){
        if(layout_object.alarm[i].type == alarm_type){
            index = i;
            break;
        }
    }

    return index;
}

int Layout::get_common_alarm()
{
     for(uint i = 0; i < SizeOfArray(layout_object); ++i){
         if(layout_object[i].alarm.size()) return 1;
     }

     return 0;
}

QString Layout::sec_to_TimeStr(qint64 sec_val)
{
    int days,hours, minutes, sec;
    days    =  sec_val / 60 / 60 / 24;
    hours   = (sec_val / 60 / 60) % 24;
    minutes = (sec_val / 60) % 60;
    sec     =  sec_val  % 60;

    QString elapsed_str;
    if((days ==0) && (hours == 0) && (minutes == 0))
        elapsed_str = QString(" - %1s").arg(sec, 2, 10, QChar('0'));
    if((days ==0) && (hours == 0) && minutes)
        elapsed_str = QString(" - %1m %2s").arg(minutes).arg(sec, 2, 10, QChar('0'));
    if((days ==0) && hours)
        elapsed_str = QString(" - %1h %2m").arg(hours).arg(minutes, 2, 10, QChar('0'));
    if(days)
        elapsed_str = QString(" - %1d %2h").arg(days).arg(hours, 2, 10, QChar('0'));

    return elapsed_str;
}

void Layout::draw_alarm_elapsed()
{
    for(int index = 0; index < 16; ++index) {
        int k = cascade.num * 16 + index;
        for(int i = 0; i < layout_object[k].alarm.size(); ++i){
            draw_alarm_label(k, layout_object[k]);
        }
    }
    mtvsystem->overlay_sync();
}


void Layout::slot_new_format()
{
    qDebug(category) << "New SDI Input Format";
    routing_source_video();
    update_alarm();

    for(int i = 0; i < 16; ++i){
        int k = cascade.num * 16 + i;
        layout_object[k].sdi_format_str = mtvsystem->get_sdi_format_str(i);

        int state = mtvsystem->get_sdi_status(i);
        led->set_led_state(i, state);
    }
}

void Layout::update_label_name_channel(int addr, QString txt)
{
QString label;
int width, height;
int x, y;

    if(!layout_object[addr].screen_plan.enable_video) return;
    if(cascade.num != (addr >> 4)) return;
    if(teletext_cell.enable && (addr == 0)) return;

    switch(layout_object[addr].cell.umd_display){
        case STATIC_ONLY:
            return;
            break;
        case UMD_ONLY:
            label = txt;
            break;
        case STATIC_AND_UMD:
            label = layout_object[addr].sdi_label + " • " + txt;
            break;
        default:
            return;
            break;
    }

    if(layout_object[addr].cell.style)  // old style
        layout_object[addr].screen_plan.panel_label = layout_object[addr].screen_old_style.panel_label.cell;

    width  = layout_object[addr].screen_plan.panel_label.width();
    height = layout_object[addr].screen_plan.panel_label.height();

    QImage image_label_panel(width, height, QImage::Format_ARGB32);
    image_label_panel.fill(0x00000000);  // partly transparent background

    x =layout_object[addr].screen_plan.panel_label.x();
    y =layout_object[addr].screen_plan.panel_label.y();

    QRect panel(0,0,width, height);

    if(layout_object[addr].cell.style)  // old style
        draw_old_style_text_panel(image_label_panel, panel, layout_object[addr].screen_old_style.panel_label.font, label);
    else
        draw_transparant_text_panel(image_label_panel, panel, FONT_CHANNEL_NAME_SIZE, Qt::AlignCenter, label);

    blit_to_frame(&image_label_panel, x, y);
    qDebug() << "update_label_name_channel";
    // flush_overlay();
    mtvsystem->draw_overlay_fast(&image_label_panel, x, y);
}

void Layout::update_sdi_format(int index)
{
    int k = cascade.num * 16 + index;
    if(!layout_object[k].screen_plan.enable_video) return;
    if(!layout_object[k].cell.sdi_format_display)  return;
    if(teletext_cell.enable && (k == 0)) return;

    int width  = layout_object[k].screen_plan.panel_format_video.width();
    int height = layout_object[k].screen_plan.panel_format_video.height() +1;

    QImage image_sdi_panel(width, height, QImage::Format_ARGB32);
    image_sdi_panel.fill(0x00000000);  // partly transparent background

    QString sdi_format = mtvsystem->get_sdi_format_str(layout_object[k].cell.input);
    draw_transparant_text_panel(image_sdi_panel, QRect(0, 0, width, height), FONT_SDI_FORMAT_SIZE, Qt::AlignRight, sdi_format);

    int x =layout_object[k].screen_plan.panel_format_video.x();
    int y =layout_object[k].screen_plan.panel_format_video.y();

    blit_to_frame(&image_sdi_panel, x, y);
    mtvsystem->draw_overlay_fast(&image_sdi_panel, x, y);
}

void Layout::flush_overlay()
{
    // system("devmem2 0xFF200004 w 0");
    // usleep(300); 
    QElapsedTimer timer;
    timer.start();  
    mtvsystem->draw_overlay(&full_overlay_frame, 0, 0);
    qDebug(category) << "mtvsystem->draw_overlay(&full_overlay_frame, 0, 0);" << timer.elapsed() << "milliseconds";
    // system("devmem2 0xFF200004 w 1");  
    qDebug(category) << "flush_overlay";
}

void Layout::blit_to_frame(QImage *image, int x, int y)
{
    QPainter painter(&full_overlay_frame);
    // Source, а не SourceOver: полностью ЗАМЕЩАЕТ пиксели и альфа-канал в этом
    // прямоугольнике, включая обнуление альфы там, где рисуемая картинка прозрачна.
    // Это и есть механизм "очистки" старого содержимого региона.
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawImage(x, y, *image);
}

void Layout::draw_overlay()
{
QImage layout_border;
static int output_format_old = -1;

    if(cascade.mode == CASCADE_SLAVE)
        output_format = OUT_STD_1080i50;
    if(output_format_old != output_format){

        if(output_format == OUT_STD_1080i50){
            //m26_control->set_v_format(mb86m26_control::PULLDOWN);
            //m26_control->set_std(STD_1080i50);
            mtvsystem->set_dei(0);
        }
        else{
            //m26_control->set_v_format(mb86m26_control::NATIVE);
            //m26_control->set_std(STD_1080p25);
            mtvsystem->set_dei(1);
        }

        output_format_old = output_format;
    }

    QElapsedTimer timer;
    timer.start();
    qDebug(category) << "====== Start Measuring ==========";
        if(solo_mode.enable)
            layout_border = get_layout_1x1(solo_mode.input);
        else
            layout_border = get_layout();

    qDebug(category) << "The get_layout_3x3 operation took" << timer.elapsed() << "milliseconds";

        
    
    blit_to_frame(&layout_border, 0, 0);

    qDebug(category) << "The blit_to_frame operation took" << timer.elapsed() << "milliseconds";

    scte_104_update();
    slot_draw_time_counter(time_counter.text);

    qDebug() << "draw_overlay";
    // Это единственное место, где пересобирается ВЕСЬ экран целиком (не точечный
    // патч) - тут двойная буферизация оправдана: собираем полную картинку в
    // full_overlay_frame -> draw_overlay() пишет её в НЕАКТИВНЫЙ буфер -> flip.
    // Так зритель не видит, как раскладка перестраивается "по кускам" вживую.
    flush_overlay();
}

void Layout::draw_overlay_test_file()
{
    QImage overlay_test_file("layout.png");
    blit_to_frame(&overlay_test_file, 0, 0);
    qDebug() << "draw_overlay_test_file";
    flush_overlay();
    // mtvsystem->draw_overlay(&overlay_test_file);
    // mtvsystem->overlay_sync();
}

void Layout::set_aspect_ratio(QRect &rec, int index)
{
const float ratio_4_3 = (float)4 / (float)3;

    int k = cascade.num * 16 + index;

    if(!layout_object[k].screen_plan.enable_video) return;

    if(mtvsystem->get_sdi_hd(index))
        return;

    if(layout_object[k].cell.aspect_ratio_sd)  // 0 - "4:3", 1 - "16:9"
        return;

    int w = (float)rec.height() * (float)ratio_4_3;
    int move = (rec.width() - w) / 2;

    rec.setWidth(w);
    rec.translate(move, 0);
}


int Layout::draw_text_icon(QImage &image,  QRect panel, QString text)
/*
 * Возвращает координату Х начала следующей иконки
*/
{
    if(text.size() == 0) return 0;

    int FontSize = 10;

    QPainter painter(&image);

    painter.setFont(QFont("Roboto", FontSize, QFont::Normal));

    QRect rec = painter.boundingRect(panel, Qt::AlignLeft, text);
    int margin_vertical   = rec.height() * 0.13;
    int margin_horizontal = margin_vertical * 2;
    rec.setRight(rec.right() + margin_horizontal*2);
    rec.setBottom(rec.bottom() + margin_vertical*2);
    rec.translate(0, margin_vertical*2);

    painter.setBrush(Qt::black);
    QPen pen;
    pen.setColor(QColor(Qt::white));
    painter.setPen(pen);
    painter.drawRect(rec);

    painter.setPen(QPen(Qt::white));
    painter.drawText(rec, Qt::AlignCenter, text);

    painter.end();

    return rec.width() + 2;
}

void Layout::draw_old_style_text_panel(QImage &image,  QRect panel, QFont font, QString text)
{
     QFontMetrics fm(font);
     float factor = (float)panel.width() / (float)fm.horizontalAdvance(text);

     if(factor < 1) // уменьшать текст если он шире окна
        font.setPointSize(font.pointSize() * factor - 1);

     QPainter painter(&image);
     painter.setFont(font);
     painter.setPen(QPen(Qt::white));

     painter.drawText(panel, Qt::AlignCenter, text);
     painter.end();
}


void Layout::draw_transparant_text_panel(QImage &image,  QRect panel, int FontSize, int AlignmentFlag, QString text)
{
    QPainter painter(&image);

    QFont font(QFont("Roboto", FontSize, QFont::Normal));
    QFontMetrics fm(font);
    float factor = (float)panel.width() / (float)fm.horizontalAdvance(text);
    if(factor < 1) // уменьшать текст если он шире окна
       font.setPointSize(font.pointSize() * factor - 1);

    painter.setFont(font);

    QRect rec = painter.boundingRect(panel, AlignmentFlag, text);
    int margin_vertical   = rec.height() * 0.14;
    int margin_horizontal = margin_vertical * 2;
    rec.setLeft( rec.left()  - margin_horizontal);
    rec.setRight(rec.right() + margin_horizontal);
    rec.setTop(   rec.top()    - margin_vertical);
    rec.setBottom(rec.bottom() + margin_vertical);
    rec.translate( 0 - margin_horizontal - 6, margin_vertical);

    QPainterPath path_frame;
    path_frame.addRoundedRect(rec, 9, 9, Qt::AbsoluteSize);
    QColor background_color;
    #define COLOR_VALUE 80
    background_color.setRgb(COLOR_VALUE, COLOR_VALUE, COLOR_VALUE, 0);
    painter.setPen(QPen(background_color));
    background_color.setAlpha(200);
    painter.setBrush(background_color);
    painter.drawPath(path_frame);

    painter.setPen(QPen(Qt::white));
    painter.drawText(rec, Qt::AlignCenter, text);

    painter.end();
}


void Layout::disable_all_audio_meters()
{
    for(int i = 0; i < 16; ++i) {
        mtvsystem->bars_configure(i, 0, 0, 0, 0, 0, 0);
    }
}


void Layout::update_layout()
{
    gpio->set_mode(gpio_mode);
    cascade_udate();
    draw_overlay();
    for(int i = 0; i < 16; ++i){
        pip_config(i);
    }

    cascade_mode_update();

    sound_routing();

    slot_qpps(); // для быстрого появления часов

    for(int i = 0; i < 16; ++i){
        int k = cascade.num * 16 + i;
        DisplayTALLY(k, layout_object[k].tally);
    }
}


void Layout::sound_routing()
{
    if(cascade.num != (solo_mode.input >> 4))
        mtvsystem->set_audio_source(15);
    else
        mtvsystem->set_audio_source(solo_mode.input & 0x0F);
}

void Layout::cascade_udate()
{
    for(uint i = 0; i < SizeOfArray(cascade_ctrl); i++){
        int enable = enable_cascade(i);
        cascade_ctrl[i]->update_config(cascade.ip[i], TCP_PORT, enable);
    }

    if(cascade.mode == STAND_ALONE) cascade.num = 0;
}


void Layout::cascade_mode_update()
/*
 *    16-й вход для режима каскадирования
 */
{
    if(cascade.mode == STAND_ALONE) return;
    if(cascade.last_slave_device)   return; // последний в цепочке. 16-й как обычный вход

    // 16-й вход на весь экран
    mtvsystem->configure_image(15, 1920, 1080, 0, 0, 1);
}


void Layout::routing_source_video()
{
static int format[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

    for(int i = 0; i < 16; ++i){
        if(format[i] != mtvsystem->get_sdi_format(i)){
            format[i] = mtvsystem->get_sdi_format(i);
            pip_config(i);
            update_sdi_format(i);
            if(i == 15) cascade_mode_update(); // чтобы не дёргался вход 8
        }
    }
    qDebug() << "routing_source_video";
    // flush_overlay();
}


void Layout::draw_grid_1x1(QImage &image)  // Full Screen Mode
{
    if(solo_mode.enable) return;

    QColor color = QColor(64,64,64);

    // Рамка большая
    QRect boundary;
    boundary.setX(0 + PEN_WIDTH);   boundary.setY(0 + PEN_WIDTH);
    boundary.setWidth(image.width()   - PEN_WIDTH * 2);
    boundary.setHeight(image.height() - PEN_WIDTH * 2);

    QPen pen;
    pen.setWidth(PEN_WIDTH * 2);
    pen.setColor(color);
    pen.setJoinStyle(Qt::MiterJoin); // чтобы не было дырок к на стыках рамок

    QPainter painter(&image);
    painter.setPen(pen);

    painter.drawRect(boundary);

    painter.end();
}


void Layout::set_audio_meter_position(int index)
{
#define SCALE_SIZE 64
int enable_1, enable_2;
int x1, x2;

    int k = cascade.num * 16 + index;

    int left   = layout_object[k].screen_plan.cell.left();
    int right  = layout_object[k].screen_plan.cell.right();
    int height = layout_object[k].screen_plan.cell.height();
    int bottom = layout_object[k].screen_plan.cell.bottom();
    height -= 15;

    x1 = left  + 7;
    x2 = right - 31;

    int scale =  height / SCALE_SIZE - 1;   // высота индикатора

    int offset = (height - (scale + 1) * SCALE_SIZE) / 2;
    int y = bottom - offset - (scale + 1) * SCALE_SIZE;

    switch(layout_object[k].cell.audio_meter){
        case 0: enable_1 = 0;
                enable_2 = 0;
                break;
        case 1:
                enable_1 = 1;
                enable_2 = 0;
                break;
        case 2:
                enable_1 = 1;
                enable_2 = 1;
                break;
        default:enable_1 = 1;
                enable_2 = 1;
                break;
    }

    if(!layout_object[k].screen_plan.enable_video){
        enable_1 = 0;
        enable_2 = 0;
    }

    int router_source = layout_object[k].cell.input;

    mtvsystem->bars_configure(router_source, x1, x2, y, scale, enable_1, enable_2);
}

QString Layout::sdi_key_name(int i)
{
    int major = (i >> 4) + 1;
    int minor = (i & 0x0F) + 1;
    return QString("%1.%2").arg(major).arg(minor);
}

void Layout::Settings_Read(){

    QSettings settings(QSettings::SystemScope, SETTINGS_SDI_INPUT_FILE_NAME);

    settings.beginGroup("sdi_input");
        for(uint i = 0; i < SizeOfArray(layout_object); i++){
            QString key_name = QString("label_%1").arg(sdi_key_name(i));
            QString default_value = QString("Input %1").arg(sdi_key_name(i));
            layout_object[i].sdi_label = settings.value(key_name, default_value).toString();
        }
    settings.endGroup();

    settings.beginGroup("solo_mode");
        solo_mode.enable = settings.value("enable", 0).toInt();
        solo_mode.input  = settings.value("input",  0).toInt();
    settings.endGroup();

    settings.beginGroup("layout_preset");
        layout_preset.index = settings.value("index", 0).toInt();
        layout_preset.name.clear();
        for(int i = 0; i < 10; ++i){
            QString key_name = QString("preset_%1").arg(i);
            QString default_value = QString("Not Set %1").arg(i + 1);
            layout_preset.name.append(settings.value(key_name, default_value).toString());
        }
    settings.endGroup();

    settings.beginGroup("audio_alarm_settings");
       alarm_settings.audio_alarm_settings.threshold     = settings.value("threshold",    5000).toInt();
       alarm_settings.audio_alarm_settings.minimum_level = settings.value("minimum_level", -80).toInt();
    settings.endGroup();

    settings.beginGroup("video_alarm_settings");
       alarm_settings.video_alarm_settings.threshold     = settings.value("threshold", 5000).toInt();
    settings.endGroup();

    settings.beginGroup("video_output_settings");
        output_format = settings.value("output_format", 1).toInt();
        hdmi_color = settings.value("hdmi_color", 1).toInt();
    settings.endGroup();

    settings.beginGroup("cascade");
        cascade.num               = settings.value("num",               0).toInt();
        cascade.mode              = settings.value("mode",              0).toInt();
        cascade.last_slave_device = settings.value("last_slave_device", 0).toInt();
        for(uint i = 0; i < SizeOfArray(cascade.ip); i++){
            QString ip_str = "ip_" + QString::number(i);
            cascade.ip[i] = settings.value(ip_str, "").toString();
        }
    settings.endGroup();

    settings.beginGroup("tally");
        gpio_mode = settings.value("gpio_mode", Gpio::SOLO).toInt();
    settings.endGroup();

} // End "Settings_Read"

void Layout::Settings_Write(){

    QSettings settings(QSettings::SystemScope, SETTINGS_SDI_INPUT_FILE_NAME);

    settings.beginGroup("sdi_input");
        for(uint i = 0; i < SizeOfArray(layout_object); i++){
            QString key_name = QString("label_%1").arg(sdi_key_name(i));
            settings.setValue(key_name, layout_object[i].sdi_label);
        }
    settings.endGroup();

    settings.beginGroup("solo_mode");
        settings.setValue("enable", solo_mode.enable);
        settings.setValue("input",  solo_mode.input);
    settings.endGroup();

    settings.beginGroup("layout_preset");
        settings.setValue("index", layout_preset.index);
        for(int i = 0; i < layout_preset.name.size(); ++i){
            QString key_name = QString("preset_%1").arg(i);
            settings.setValue(key_name, layout_preset.name[i]);
        }
    settings.endGroup();

    settings.beginGroup("audio_alarm_settings");
        settings.setValue("threshold",     alarm_settings.audio_alarm_settings.threshold    );
        settings.setValue("minimum_level", alarm_settings.audio_alarm_settings.minimum_level);
    settings.endGroup();

    settings.beginGroup("video_alarm_settings");
        settings.setValue("threshold",     alarm_settings.video_alarm_settings.threshold);
    settings.endGroup();

    settings.beginGroup("video_output_settings");
        settings.setValue("output_format", output_format);
        settings.setValue("hdmi_color", hdmi_color);
    settings.endGroup();

    settings.beginGroup("cascade");
        settings.setValue("num",               cascade.num              );
        settings.setValue("mode",              cascade.mode             );
        settings.setValue("last_slave_device", cascade.last_slave_device);
        for(uint i = 0; i < SizeOfArray(cascade.ip); i++){
            QString ip_str = "ip_" + QString::number(i);
            settings.setValue(ip_str, cascade.ip[i]);
        }
    settings.endGroup();

    settings.beginGroup("tally");
        settings.setValue("gpio_mode", gpio_mode);
    settings.endGroup();

} // End "Settings_Write"

void Layout::Clear_SDI_input_Settings_File()
{
    QSettings settings(QSettings::SystemScope, SETTINGS_SDI_INPUT_FILE_NAME);
    settings.clear();
} // End "Clear_SDI_input_Settings_File"

QString Layout::get_preset_file_name(int preset_num)
{
    QString file_name = LAYOUT_PRESET_FILE_NAME;
    file_name +="_" + QString::number(preset_num);
    //qDebug(category) << "\t\tget_preset_file_name" << file_name;

    return file_name;
}

void Layout::preset_Layout_Read(int preset_num){
const int x[] = { 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3};
const int y[] = { 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3};

    QString file_name = get_preset_file_name(preset_num);

    QSettings settings(QSettings::SystemScope, file_name); // settings init

    for(uint i = 0; i < SizeOfArray(layout_object); ++i){
        int k = i & 0x0F;
        int enable = i < 16;
        QString name_group = "cell_" + QString::number(i);
        settings.beginGroup(name_group);
            layout_object[i].cell.border_color       = settings.value("border_color",             0).toInt();
            layout_object[i].cell.sdi_format_display = settings.value("sdi_format_display",       1).toInt();
            layout_object[i].cell.teletext_icon_display = settings.value("teletext_icon_display", 1).toInt(); 
            layout_object[i].cell.umd_display        = settings.value("umd_display", STATIC_AND_UMD).toInt();
            layout_object[i].cell.enable_label       = settings.value("enable_label",             1).toInt();
            layout_object[i].cell.audio_meter        = settings.value("audio_meter",              1).toInt();
            for(uint k = 0; k < SizeOfArray(layout_object[i].cell.audio_alarm_channel_enable); ++k){
                QString name_var = "audio_alarm_channel_enable_" + QString::number(k);
                layout_object[i].cell.audio_alarm_channel_enable[k] = settings.value(name_var,    0).toInt();
            }
            layout_object[i].cell.input              = settings.value("input",                    k).toInt();
            layout_object[i].cell.scale_x            = settings.value("scale_x",                  0).toInt();
            layout_object[i].cell.scale_y            = settings.value("scale_y",                  0).toInt();
            layout_object[i].cell.enable             = settings.value("enable",              enable).toInt();
            layout_object[i].cell.x                  = settings.value("x",                     x[k]).toInt();
            layout_object[i].cell.y                  = settings.value("y",                     y[k]).toInt();
            layout_object[i].cell.audio_alarm.enable = settings.value("audio_alarm_enable",       1).toInt();
            layout_object[i].cell.video_alarm.enable = settings.value("video_alarm_enable",       1).toInt();
            layout_object[i].cell.scte_104_display   = settings.value("scte_104_display",         0).toInt();
            layout_object[i].cell.aspect_ratio_sd    = settings.value("aspect_ratio_sd",          0).toInt();
            layout_object[i].cell.style              = settings.value("cell_style",               0).toInt();
        settings.endGroup();
    }

    for(uint i = 0; i < SizeOfArray(label_cell); ++i){
        QString name_group = "label_" + QString::number(i);
        settings.beginGroup(name_group);
            label_cell[i].scale_x    = settings.value("scale_x",  0).toInt();
            label_cell[i].scale_y    = settings.value("scale_y",  0).toInt();
            label_cell[i].enable     = settings.value("enable",   0).toInt();
            label_cell[i].x          = settings.value("x",        0).toInt();
            label_cell[i].y          = settings.value("y",        0).toInt();
            label_cell[i].label.text = settings.value("text",    "").toString();
        settings.endGroup();
    }

    settings.beginGroup("common_setting");
        grid   = settings.value("grid",   GRID_4x4).toInt();
    settings.endGroup();

    settings.beginGroup("clock_cell");
        clock_cell.enable      = settings.value("enable",         0).toInt();
        clock_cell.scale_x     = settings.value("scale_x",        0).toInt();
        clock_cell.scale_y     = settings.value("scale_y",        0).toInt();
        clock_cell.x           = settings.value("x",              2).toInt();
        clock_cell.y           = settings.value("y",              2).toInt();
        clock_cell.style       = settings.value("style",          0).toInt();
        clock_cell.label       = settings.value("label",   "Moscow").toString();
        clock_cell.date_locale = settings.value("date_locale",    0).toInt();
    settings.endGroup();

    settings.beginGroup("time_counter_cell");
        time_counter_cell.enable  = settings.value("enable",         0).toInt();
        time_counter_cell.scale_x = settings.value("scale_x",        0).toInt();
        time_counter_cell.scale_y = settings.value("scale_y",        0).toInt();
        time_counter_cell.x       = settings.value("x",              2).toInt();
        time_counter_cell.y       = settings.value("y",              2).toInt();
    settings.endGroup();

    settings.beginGroup("teletext_cell");
        teletext_cell.enable  = settings.value("enable",         0).toInt();
        teletext_cell.page    = settings.value("page",         888).toInt();
    settings.endGroup();
    //qDebug(category) << "\t\tpreset read done keys" << settings.allKeys().size(); 

} // End "Layout_preset_Read"

void Layout::preset_Layout_Write(int preset_num){

    QString file_name = get_preset_file_name(preset_num);

    QSettings settings(QSettings::SystemScope, file_name);

    for(uint i = 0; i < SizeOfArray(layout_object); ++i){
        QString name_group = "cell_" + QString::number(i);
        settings.beginGroup(name_group);
            settings.setValue("border_color",       layout_object[i].cell.border_color );
            settings.setValue("sdi_format_display", layout_object[i].cell.sdi_format_display);
            settings.setValue("teletext_icon_display", layout_object[i].cell.teletext_icon_display);   
            settings.setValue("umd_display",        layout_object[i].cell.umd_display);
            settings.setValue("enable_label",       layout_object[i].cell.enable_label );
            settings.setValue("audio_meter",        layout_object[i].cell.audio_meter  );
            for(uint k = 0; k < SizeOfArray(layout_object[i].cell.audio_alarm_channel_enable); ++k){
                QString name_var = "audio_alarm_channel_enable_" + QString::number(k);
                settings.setValue(name_var,        layout_object[i].cell.audio_alarm_channel_enable[k]);
            }
            settings.setValue("input",              layout_object[i].cell.input        );
            settings.setValue("scale_x",            layout_object[i].cell.scale_x      );
            settings.setValue("scale_y",            layout_object[i].cell.scale_y      );
            settings.setValue("enable",             layout_object[i].cell.enable       );
            settings.setValue("x",                  layout_object[i].cell.x            );
            settings.setValue("y",                  layout_object[i].cell.y            );
            settings.setValue("audio_alarm_enable", layout_object[i].cell.audio_alarm.enable);
            settings.setValue("video_alarm_enable", layout_object[i].cell.video_alarm.enable);
            settings.setValue("scte_104_display",   layout_object[i].cell.scte_104_display);
            settings.setValue("aspect_ratio_sd",    layout_object[i].cell.aspect_ratio_sd);
            settings.setValue("cell_style",         layout_object[i].cell.style);
        settings.endGroup();
    }

    for(uint i = 0; i < SizeOfArray(label_cell); ++i){
        QString name_group = "label_" + QString::number(i);
        settings.beginGroup(name_group);
            settings.setValue("scale_x", label_cell[i].scale_x   );
            settings.setValue("scale_y", label_cell[i].scale_y   );
            settings.setValue("enable",  label_cell[i].enable    );
            settings.setValue("x",       label_cell[i].x         );
            settings.setValue("y",       label_cell[i].y         );
            settings.setValue("text",    label_cell[i].label.text);
        settings.endGroup();
    }

    settings.beginGroup("common_setting");
         settings.setValue("grid",  grid);
    settings.endGroup();

    settings.beginGroup("clock_cell");
         settings.setValue("enable",      clock_cell.enable     );
         settings.setValue("scale_x",     clock_cell.scale_x    );
         settings.setValue("scale_y",     clock_cell.scale_y    );
         settings.setValue("x",           clock_cell.x          );
         settings.setValue("y",           clock_cell.y          );
         settings.setValue("style",       clock_cell.style      );
         settings.setValue("label",       clock_cell.label      );
         settings.setValue("date_locale", clock_cell.date_locale);
    settings.endGroup();

    settings.beginGroup("time_counter_cell");
         settings.setValue("enable",  time_counter_cell.enable );
         settings.setValue("scale_x", time_counter_cell.scale_x);
         settings.setValue("scale_y", time_counter_cell.scale_y);
         settings.setValue("x",       time_counter_cell.x      );
         settings.setValue("y",       time_counter_cell.y      );
    settings.endGroup();

    settings.beginGroup("teletext_cell");
         settings.setValue("enable",  teletext_cell.enable);
         settings.setValue("page",    teletext_cell.page  );
    settings.endGroup();

} // End "Layout_preset_Write"

void Layout::set_cell_plane(layout_object_t &cell_object)
{
int width, height;

    width  =  width_grid;
    height =  height_grid;

    int pos_x = width  * cell_object.cell.x;
    int pos_y = height * cell_object.cell.y;

    width  *= cell_object.cell.scale_x + 1;
    height *= cell_object.cell.scale_y + 1;

    cell_object.screen_plan.cell.setRect( pos_x, pos_y, width, height);

    cell_object.screen_plan.plane_video = cell_object.screen_plan.cell;

    set_cell_plane_old_style(cell_object);

    set_info_panel_size(cell_object);
}

void Layout::set_cell_plane_old_style(layout_object_t &cell_object)
{
#define OLD_STYLE_BAR 37
#define OLD_STYLE_PANEL_SIZE_H (float(720) / (float)640 * OLD_STYLE_BAR)

    if(cell_object.cell.style == 0) return;

    QRect pip = cell_object.screen_plan.cell;

    pip.setLeft(pip.left() + OLD_STYLE_BAR);
    pip.setRight(pip.right() - OLD_STYLE_BAR);
    pip.setBottom(pip.bottom() - OLD_STYLE_PANEL_SIZE_H);

    // Положение Red Tally
    cell_object.screen_old_style.panel_tally_red.setLeft(pip.left());
    cell_object.screen_old_style.panel_tally_red.setTop(pip.y() + pip.height());
    cell_object.screen_old_style.panel_tally_red.setHeight(OLD_STYLE_PANEL_SIZE_H);
    cell_object.screen_old_style.panel_tally_red.setWidth(OLD_STYLE_PANEL_SIZE_H);

    // Положение Green Tally
    cell_object.screen_old_style.panel_tally_green = cell_object.screen_old_style.panel_tally_red;
    cell_object.screen_old_style.panel_tally_green.moveRight(pip.right());

    // Название канала
    cell_object.screen_old_style.panel_label.cell = cell_object.screen_old_style.panel_tally_red;
    cell_object.screen_old_style.panel_label.cell.setRight(cell_object.screen_old_style.panel_tally_green.left());
    cell_object.screen_old_style.panel_label.cell.setLeft(cell_object.screen_old_style.panel_tally_red.right());
    QFont font(QFont("Roboto", FONT_CHANNEL_NAME_SIZE, QFont::Normal));
    cell_object.screen_old_style.panel_label.font = font;

    cell_object.screen_plan.plane_video = pip;
}

void Layout::get_grid_size()
{
    switch(grid){
        case GRID_3x3:
            width_grid  =  WIDTH_3x3;
            height_grid = HEIGHT_3x3;
            break;
        case GRID_4x4:
            width_grid  =  WIDTH_4x4;
            height_grid = HEIGHT_4x4;
            break;
        case GRID_5x5:
            width_grid  =  WIDTH_5x5;
            height_grid = HEIGHT_5x5;
            break;
        case GRID_6x6:
            width_grid  =  WIDTH_6x6;
            height_grid = HEIGHT_6x6;
            break;
        case GRID_12x12:
            width_grid  =  WIDTH_12x12;
            height_grid = HEIGHT_12x12;
            break;
        default:
            width_grid  = 0;
            height_grid = 0;
            break;
    }
}


QImage Layout::get_layout()
{
    get_grid_size();

    set_time_counter_plane();
    set_label_plane();
    set_layout_plane();
    set_teletext_plane(layout_object[0].screen_plan.cell);
    set_clock_plane();

    if(STAND_ALONE < cascade.mode && cascade.mode < CASCADE_SLAVE)
        layout_object[15].cell.enable = 0;

    QImage image(1920, 1080, QImage::Format_ARGB32);

    for(int i = 0; i < 16; ++i){
        int k = cascade.num * 16 + i;
        layout_object[k].screen_plan.enable_video = layout_object[k].cell.enable;
        if(layout_object[k].screen_plan.enable_video)
            draw_layout_object(image, layout_object[k]);
    }

    for(int i = 0; i < 16; ++i){
        int k = cascade.num * 16 + i;
        if(label_cell[k].enable)
            draw_label_object(image, label_cell[k]);
    }


    if(clock_cell.style)
        draw_layout_digital_clock(image);
    else
        draw_layout_analog_clock(image);

    draw_time_counter_frame(image);
    draw_teletext_frame(image);

    return image;
}

void Layout::draw_teletext_frame(QImage &image)
{
    if(!teletext_cell.enable) return;
    if(solo_mode.enable)   return;

    draw_frame(image, QColor(64,64,64), teletext_cell.frame, PEN_WIDTH * 2);
}

void Layout::draw_time_counter_frame(QImage &image)
{
    if(!time_counter_cell.enable) return;
    if(solo_mode.enable)   return;

    draw_frame(image, QColor(64,64,64), time_counter_cell.frame, PEN_WIDTH * 2);
}

void Layout::draw_layout_analog_clock(QImage &image)
{
    if(!clock_cell.enable) return;
    if(solo_mode.enable)   return;

    int FontSize = analog_clock.label_rec.height() - analog_clock.label_rec.height() * 0.55;
    QPainter painter(&image);

    painter.setFont(QFont("Roboto", FontSize, QFont::Normal));

    painter.setPen(QPen(Qt::white));
    painter.drawText(analog_clock.label_rec, Qt::AlignCenter, clock_cell.label);

    painter.end();
}

void Layout::draw_layout_digital_clock(QImage &image)
{
    if(!clock_cell.enable) return;
    if(solo_mode.enable)   return;

    int FontSize = digital_clock.label_rec.height() * 0.50;
    QPainter painter(&image);

    painter.setFont(QFont("Roboto", FontSize, QFont::Normal));

    painter.setPen(QPen(Qt::white));
    painter.drawText(digital_clock.label_rec, Qt::AlignCenter, clock_cell.label);

    painter.end();
}

void Layout::set_teletext_plane(QRect cell)
{
#define MARGIN 20
    QRect pip = cell;

    pip.setWidth(pip.width() - MARGIN);

    get_ratio_492_250(pip);
    pip.moveCenter(cell.center());
    teletext_cell.panel_teletext = pip;
}

void Layout::get_ratio_492_250(QRect &pip)
/*
 * Размер изображения телетекста. Размер взят из
 * функции: "QImage TeletextDecoder::get_page(int page)".
*/
{
    // 250 -> 230 что бы учесть 2 строки текста по 10px
    const float ratio_492_250 = (float)492 / (float)230;
    float ratio = (float)pip.width() / (float)pip.height();

    if(ratio > ratio_492_250){
        int w = (float)pip.height() * (float)ratio_492_250;
        pip.setWidth(w);
    }
    else{
        int h = (float)pip.width() / (float)ratio_492_250;
        pip.setHeight(h);
    }

}

void Layout::get_ratio_4_3(QRect &pip)
{
    const float ratio_4_3 = (float)4 / (float)3;
    float ratio = (float)pip.width() / (float)pip.height();

    if(ratio > ratio_4_3){
        int w = (float)pip.height() * (float)ratio_4_3;
        pip.setWidth(w);
    }
    else{
        int h = (float)pip.width() / (float)ratio_4_3;
        pip.setHeight(h);
    }

}

void Layout::set_time_counter_plane()
{
int width, height;

    width  = width_grid;
    height = height_grid;

    int pos_x = width  * time_counter_cell.x;
    int pos_y = height * time_counter_cell.y;

    width  *= time_counter_cell.scale_x + 1;
    height *= time_counter_cell.scale_y + 1;

    time_counter_cell.frame.setRect(pos_x, pos_y, width, height);

    QRect rec;
    rec.setRect(pos_x, pos_y, width, height);

    // Положение счётчика времени при произвольном размере окон
    time_counter.cell = rec;

    float w = (float)width / (float)height;
    if(w >= 5)
        time_counter.cell.setWidth(rec.height() * 5);
    else
        time_counter.cell.setHeight(rec.width() / 5);

    time_counter.cell.moveCenter(rec.center());

    // Размер счётчика времени
    time_counter.size = time_counter.cell;
    int width_time_counter = time_counter.cell.width() * 0.80;
    time_counter.size.setWidth(width_time_counter);
    int height_time_counter = time_counter.cell.height() * 0.80;
    time_counter.size.setHeight(height_time_counter);

    time_counter.size.moveCenter(time_counter.cell.center());

    int FontSize = time_counter.size.height() * 0.90;
    time_counter.font.setPointSize(FontSize);
    time_counter.font.setWeight(QFont::Normal);
    time_counter.font.setFamily("Roboto");
}

void Layout::set_clock_plane()
{
int width, height;

    width  = width_grid;
    height = height_grid;

    int pos_x = width  * clock_cell.x;
    int pos_y = height * clock_cell.y;

    width  *= clock_cell.scale_x + 1;
    height *= clock_cell.scale_y + 1;

    QRect rec;
    rec.setRect(pos_x, pos_y, width, height);

    // Положение цифровых часов при произвольном размере окон
    digital_clock.cell = rec;
    float ratio;
    if(clock_cell.label.isEmpty())
        ratio = 3.0;
    else
        ratio = 1.87;

    float w = (float)width / (float)height;
    if(w >= ratio)
        digital_clock.cell.setWidth(rec.height() * ratio);
    else
        digital_clock.cell.setHeight(rec.width() / ratio);

    int width_digital_clock = digital_clock.cell.width() * 0.90;
    digital_clock.cell.setWidth(width_digital_clock);
    int height_digital_clock = digital_clock.cell.height() * 0.80;
    digital_clock.cell.setHeight(height_digital_clock);

    digital_clock.cell.moveCenter(rec.center());

    // Положение аналоговых часов при произвольном размере окон
    analog_clock.cell = rec;
    int side = qMin(analog_clock.cell.width(), analog_clock.cell.height());
    analog_clock.cell.setWidth(side);
    analog_clock.cell.setHeight(side);
    analog_clock.cell.moveCenter(rec.center());

    // Размеры аналоговых часов
    analog_clock.clock_size = analog_clock.cell;

    int bottom = analog_clock.cell.bottom();
    if(clock_cell.label.isEmpty()){
        bottom -= analog_clock.cell.height() * 0.05;
    }
    else{
        bottom -= analog_clock.cell.height() * 0.15;
    }
    analog_clock.clock_size.setBottom(bottom);

    int top = analog_clock.cell.top();
    top += analog_clock.cell.height() * 0.05;
    analog_clock.clock_size.setTop(top);

    // Размеры окна надписи аналоговых часов
    analog_clock.label_rec = analog_clock.cell;
    analog_clock.label_rec.setTop(analog_clock.clock_size.bottom());

    // Размер цифровых часов
    digital_clock.clock_size = digital_clock.cell;
    int width_digit_clock = digital_clock.cell.width();

    digital_clock.clock_size.moveCenter(digital_clock.cell.center());

    float to_center, kof_h;
    if(clock_cell.label.isEmpty()){
        to_center = 0.00;
        kof_h = 0.75;
    }
    else{
        to_center = 0.15;
        kof_h = 0.49;
    }

    digital_clock.clock_size.translate(0, digital_clock.clock_size.height() * to_center);
    digital_clock.clock_size.setHeight(digital_clock.clock_size.height() * kof_h);

    // Размер окна даты
    digital_clock.date_rec = digital_clock.cell;
    digital_clock.date_rec.setWidth(width_digit_clock);
    digital_clock.date_rec.moveCenter(digital_clock.cell.center());

    digital_clock.date_rec.setTop(digital_clock.clock_size.bottom());
    digital_clock.date_rec.setHeight(digital_clock.clock_size.height() * 0.40);

    // Размер окна label под цифровыми часами
    digital_clock.label_rec = digital_clock.cell;
    digital_clock.label_rec.setTop(digital_clock.date_rec.bottom());
    digital_clock.label_rec.setHeight(digital_clock.date_rec.height() - digital_clock.date_rec.height() * 0.05);
}

void Layout::slot_draw_time_counter(QString time_counter_str)
{
    if(!time_counter_cell.enable) return;

    time_counter.text = time_counter_str;
    draw_time_counter(time_counter);
}

void Layout::draw_time_counter(label_t label)
{
    if(solo_mode.enable) return;

    QRect time_counter_rec = label.size;
    time_counter_rec.moveTo(0,0);
    QImage image_time_counter(time_counter_rec.width(), time_counter_rec.height(), QImage::Format_ARGB32);
    image_time_counter.fill(Qt::black);

    QPainter painter(&image_time_counter);
    painter.setFont(label.font);
    painter.setPen(QPen(Qt::white));

    painter.drawText(time_counter_rec, Qt::AlignCenter, label.text);
    painter.end();

    blit_to_frame(&image_time_counter, label.size.x(), label.size.y());
    // Быстрый путь: сразу в активный HW-буфер, не дожидаясь раз-в-секунду flush_overlay().
    // Это и нужно, чтобы десятые доли секунды реально обновлялись на экране 10 раз/сек.
    mtvsystem->draw_overlay_fast(&image_time_counter, label.size.x(), label.size.y());
}

void Layout::draw_digital_clock()
{
QString date_str;

    if(!clock_cell.enable) return;
    if(solo_mode.enable)   return;

    QString digital_clock_str = QTime::currentTime().toString("hh:mm:ss");

    int FontSize = digital_clock.clock_size.height() * 0.75;

    QFont clock_font("Roboto", FontSize, QFont::Normal);

    QRect clock_rec = digital_clock.clock_size;
    clock_rec.moveTo(0,0);
    QImage image_clock(clock_rec.width(), clock_rec.height(), QImage::Format_ARGB32);
    image_clock.fill(Qt::transparent);
    QPainter painter(&image_clock);
    painter.setFont(clock_font);
    painter.setPen(QPen(Qt::white));

    painter.drawText(clock_rec, Qt::AlignCenter, digital_clock_str);
    painter.end();
    blit_to_frame(&image_clock, digital_clock.clock_size.x(),  digital_clock.clock_size.y());
    mtvsystem->draw_overlay_fast(&image_clock, digital_clock.clock_size.x(),  digital_clock.clock_size.y());

    QRect date_rec = digital_clock.date_rec;
    date_rec.moveTo(0,0);
    QImage image_date(date_rec.width(), date_rec.height(), QImage::Format_ARGB32);
    image_date.fill(Qt::transparent);
    QPainter painter_date(&image_date);

    int Date_FontSize = digital_clock.date_rec.height() * 0.45;
    QFont date_font("Roboto", Date_FontSize, QFont::Normal);

    painter_date.setFont(date_font);
    painter_date.setPen(QPen(Qt::gray));

    QDateTime date = QDateTime::currentDateTime();
    QLocale russian(QLocale::Russian);
    if(clock_cell.date_locale)
        date_str = russian.toString(date,"d MMMM yyyy");
    else
        date_str = date.toString("d MMMM yyyy");

    painter_date.drawText(date_rec, Qt::AlignCenter, date_str);
    blit_to_frame(&image_date, digital_clock.date_rec.x(),  digital_clock.date_rec.y());
    mtvsystem->draw_overlay_fast(&image_date, digital_clock.date_rec.x(),  digital_clock.date_rec.y());

    mtvsystem->overlay_sync();
}

void Layout::slot_draw_analog_clock_tick()
{
    // Пока выбран стиль "аналоговые часы" - перерисовываем чаще 1 раза в секунду,
    // чтобы секундная стрелка двигалась плавно, а не "прыжками".
    if(!clock_cell.style)
        draw_analog_clock();
}

void Layout::draw_analog_clock()
{
    if(!clock_cell.enable) return;
    if(solo_mode.enable)   return;

    QRect clock_rec = analog_clock.clock_size;
    int side = qMin(clock_rec.width(), clock_rec.height());
    side &= ~0x07;
    clock_rec.translate((analog_clock.cell.width() - side)/2, 0);

    // Циферблат (риски/цифры) не меняется от тика к тику - рендерим SVG только
    // при первом вызове или если поменялся размер (сменилась раскладка), а не
    // на каждый вызов таймера. Дальше просто копируем готовую картинку (Qt
    // copy-on-write - это дёшево, реальное копирование произойдёт только когда
    // мы начнём рисовать поверх).
    if(m_clock_face_cache.width() != side) {
        m_clock_face_cache = QImage(side, side, QImage::Format_ARGB32);
        m_clock_face_cache.fill(0x00000000);
        QPainter face_painter(&m_clock_face_cache);
        face_painter.setRenderHint(QPainter::Antialiasing);
        m_clock_face.render(&face_painter);
        face_painter.end();
    }

    QImage image_clock = m_clock_face_cache;

    QPainter painter(&image_clock);
    painter.setRenderHint(QPainter::Antialiasing);

    QTime time = QTime::currentTime();
    // Дробные секунды (с учётом миллисекунд) - на них и держится плавность хода стрелки
    double sec_frac = time.second() + time.msec() / 1000.0;

    double cx = image_clock.width()  / 2.0;
    double cy = image_clock.height() / 2.0;
    double r  = image_clock.width()  / 2.0;

    // Рисуем стрелку как заострённый к концу многоугольник вместо SVG-рендера:
    // быстрее (нет разбора векторного документа) и даёт полный контроль над формой.
    auto draw_hand = [&](double angle_deg, double length_ratio, double width, const QColor &color, double tail_ratio)
    {
        painter.save();
        painter.translate(cx, cy);
        painter.rotate(angle_deg);

        double len  = r * length_ratio;
        double tail = r * tail_ratio;

        QPainterPath path;
        path.moveTo(-width / 2.0, tail);
        path.lineTo(-width / 4.0, -len + width);
        path.lineTo(0,            -len);
        path.lineTo( width / 4.0, -len + width);
        path.lineTo( width / 2.0, tail);
        path.closeSubpath();

        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawPath(path);
        painter.restore();
    };

    const QColor hand_color(30, 30, 30);
    const QColor second_hand_color(190, 30, 30);

    // Часовая - короткая и толстая
    draw_hand(30.0 * ((time.hour() % 12) + time.minute() / 60.0), 0.50, r * 0.07, hand_color, 0.0);
    // Минутная - длиннее и тоньше
    draw_hand(6.0  * (time.minute() + sec_frac / 60.0),           0.75, r * 0.04, hand_color, 0.0);
    // Секундная - тонкая, с небольшим "противовесом"-хвостиком, как у настоящих часов
    draw_hand(6.0 * sec_frac,                                     0.85, r * 0.025, second_hand_color, 0.15);

    // Ось в центре
    painter.setPen(Qt::NoPen);
    painter.setBrush(second_hand_color);
    painter.drawEllipse(QPointF(cx, cy), r * 0.025, r * 0.025);

    painter.end();

    blit_to_frame(&image_clock, clock_rec.x(),  clock_rec.y());
    // Быстрый путь: сразу в активный HW-буфер (без flip) - FPGA подхватит
    // изменение стрелок в пределах ~16мс благодаря slot_fps_hardware_trigger().
    mtvsystem->draw_overlay_fast(&image_clock, clock_rec.x(), clock_rec.y());
}


void Layout::draw_frame(QImage &image, QColor color, QRect boundary, int width)
{
    QPen pen;
    pen.setWidth(width);
    pen.setColor(color);
    pen.setJoinStyle(Qt::MiterJoin); // чтобы не было дырок на стыках рамок

    QPainter painter(&image);
    painter.setPen(pen);

    painter.drawRect(boundary);
    painter.end();
}

void Layout::DisplayTALLY(int addr, int tally)
{
    if(!layout_object[addr].screen_plan.enable_video) return;

    if(cascade.num == (addr >> 4)){
        slot_TALLY(addr & 0x0F, tally);
    }
}

void Layout::slot_TALLY(int input, int state)
{
    if(!cascade.last_slave_device && (input == 15)) return ;

    int k = cascade.num * 16 + input;
    layout_object[k].tally = state;

    if(!layout_object[k].screen_plan.enable_video) return;

    if(layout_object[k].cell.style)
        draw_TALLY_indicator_old_style(state, layout_object[k]);
    else
        draw_TALLY_indicator(state, layout_object[k]);
    
    qDebug() << "slot_TALLY";
    // flush_overlay();
}

void Layout::draw_TALLY_indicator_old_style(int state, const layout_object_t &layout_object)
{
QColor Ind_RED_color, Ind_GREEN_color;

    switch(state) {
        case TALLY_OFF:
            Ind_RED_color   = TALLY_RED_OFF_COLOR;
            Ind_GREEN_color = TALLY_GREEN_OFF_COLOR;
            break;
        case TALLY_RED:
            Ind_RED_color   = Qt::red;
            Ind_GREEN_color = TALLY_GREEN_OFF_COLOR;
            break;
        case TALLY_GREEN:
            Ind_RED_color   = TALLY_RED_OFF_COLOR;
            Ind_GREEN_color = Qt::green;
            break;
        case TALLY_RED_AND_GREEN:
            Ind_RED_color   = Qt::red;
            Ind_GREEN_color = Qt::green;
            break;
        default:
            Ind_RED_color   = TALLY_RED_OFF_COLOR;
            Ind_GREEN_color = TALLY_GREEN_OFF_COLOR;
            break;
    }

    draw_TALLY_indicator_old_style(layout_object.screen_old_style.panel_tally_red,   Ind_RED_color);
    draw_TALLY_indicator_old_style(layout_object.screen_old_style.panel_tally_green, Ind_GREEN_color);
}


void Layout::clear_TALLY_indicator(layout_object_t layout_object)
{
    if(!layout_object.screen_plan.enable_video) return;

    QRect cell = layout_object.screen_plan.cell;

    QRect clear_rec = get_TALLY_indicator_rec(cell);

    QImage image_clear(clear_rec.width(), clear_rec.height(),  QImage::Format_ARGB32);
    image_clear.fill(0);
    blit_to_frame(&image_clear, clear_rec.x(),  clear_rec.y());
    mtvsystem->overlay_sync();
}

void Layout::draw_TALLY_indicator_old_style(QRect cell, QColor color)
{
    QImage image_tally(cell.width(), cell.height() - 2,  QImage::Format_ARGB32);
    image_tally.fill(Qt::black);

    QRect tally_cell = QRect(0, 0, cell.width(), cell.height());
    get_image_TALLY_indicator_old_style(image_tally, tally_cell, color);

    blit_to_frame(&image_tally, cell.x(), cell.y());
    mtvsystem->draw_overlay_fast(&image_tally, cell.x(), cell.y());
    mtvsystem->overlay_sync();
}

void Layout::get_image_TALLY_indicator_old_style(QImage &image, const QRect &cell, const QColor &color)
{
    int w = cell.width() * 0.6;
    int h = cell.height() * 0.6;

    QRect tally_rect;
    tally_rect.setWidth(w);
    tally_rect.setHeight(h);
    tally_rect.moveCenter(cell.center());

    QPainter painter_tally(&image);
    painter_tally.setBrush(color);
    painter_tally.drawRoundedRect(tally_rect, 2, 2, Qt::AbsoluteSize);
    painter_tally.end();
}

void Layout::draw_TALLY_indicator(int tally_indicator_mode, const layout_object_t &layout_object)
{
QRect tally_rec;
QRect cell;
QColor color;

    if(tally_indicator_mode == TALLY_OFF){
        clear_TALLY_indicator(layout_object);
        return;
    }

    switch(tally_indicator_mode) {
        case TALLY_RED:
            color = Qt::red;
            break;
        case TALLY_GREEN:
            color = Qt::green;
            break;
        case TALLY_RED_AND_GREEN:
            color = Qt::red;
            break;
        default:
            color = Qt::red;
            break;
    }

    cell = layout_object.screen_plan.cell;
    tally_rec = get_TALLY_indicator_rec(cell);

    QImage image_tally(tally_rec.width(), tally_rec.height(),  QImage::Format_ARGB32);
    QRect alarm_rec;
    alarm_rec = QRect(0, 0, tally_rec.width(), tally_rec.height());

    image_tally.fill(0);
    QPainter painter_tally(&image_tally);
    painter_tally.setBrush(color);

    painter_tally.drawRoundedRect(alarm_rec, 5, 5, Qt::AbsoluteSize);

    if(tally_indicator_mode == TALLY_RED_AND_GREEN){
        QRect green_half_rec = alarm_rec;
        green_half_rec.setLeft(green_half_rec.left() + green_half_rec.width() / 2 );
        painter_tally.setBrush(Qt::green);
        painter_tally.drawRoundedRect(green_half_rec, 5, 5, Qt::AbsoluteSize);

        QRect splice = green_half_rec;
        splice.setWidth(5);
        painter_tally.setCompositionMode(QPainter::CompositionMode_Clear);
        painter_tally.eraseRect(splice);

        painter_tally.end();
    }

    blit_to_frame(&image_tally, tally_rec.x(), tally_rec.y());
    mtvsystem->draw_overlay_fast(&image_tally, tally_rec.x(), tally_rec.y());
    mtvsystem->overlay_sync();
}

QRect Layout::get_TALLY_indicator_rec(QRect cell)
{
    QRect tally_rec = cell;

    tally_rec.setHeight(10);
    tally_rec.setWidth(cell.width() * 0.40);

    tally_rec.translate(cell.width() / 2 -  tally_rec.width() / 2, 10);

    return tally_rec;
}


void Layout::draw_TALLY_frame(QImage &image, QColor color, QRect boundary)
{
#define TALLY_PEN_WIDTH 3
    QPen pen;
    pen.setWidth(TALLY_PEN_WIDTH * 2);
    pen.setColor(color);
    pen.setJoinStyle(Qt::MiterJoin); // чтобы не было дырок на стыках рамок

    QPainter painter(&image);
    painter.setPen(pen);


    QPoint center = boundary.center();

    boundary.setWidth( boundary.width()  - TALLY_PEN_WIDTH * 4 - 2);
    boundary.setHeight(boundary.height() - TALLY_PEN_WIDTH * 4 - 2);
    boundary.moveCenter(center);

    painter.drawRect(boundary);
    painter.end();
}

void Layout::pip_config(int i)
{
    int k = cascade.num * 16 + i;
    QRect pip = layout_object[k].screen_plan.plane_video;
    int router_source = layout_object[k].cell.input;
    int enable = layout_object[k].screen_plan.enable_video;

    const float ratio_16_9 = (float)16 / (float)9;
    float ratio = (float)pip.width() / (float)pip.height();

    if(ratio > ratio_16_9){
        int w = (float)pip.height() * (float)ratio_16_9;
        int move = (pip.width() - w) / 2 + 3;

        pip.setWidth(w);
        pip.translate(move, 0);
    }
    else{
        int h = (float)pip.width() / (float)ratio_16_9;
        int move = (pip.height() - h) / 2;

        pip.setHeight(h);
        pip.translate(0, move);
    }

    set_aspect_ratio(pip, i);

    mtvsystem->configure_image(router_source,
                               pip.width(),
                               pip.height(),
                               pip.x(),
                               pip.y(),
                               enable);

    set_audio_meter_position(i);
}

QImage Layout::get_layout_1x1(int index)
{
    QImage image(1920, 1080, QImage::Format_ARGB32);

    QRect cell(0, 0, image.width(), image.height());
    set_teletext_plane(cell);

    for(int i = 0; i < 16; ++i){
        int k = cascade.num * 16 + i;
        layout_object[k].screen_plan.enable_video = 0;
    }

    if((index >> 4) != cascade.num) return image;

    int k = cascade.num * 16 + (index & 0x0F);
    layout_object[k].screen_plan.enable_video = 1;
    layout_object[k].screen_plan.cell = cell;
    layout_object[k].screen_plan.plane_video = cell;

    set_cell_plane_old_style(layout_object[k]);

    set_info_panel_size(layout_object[k]);

    draw_layout_object(image, layout_object[k]);

    return image;
}

void Layout::set_info_panel_size(layout_object_t &cell)
{
    // Панель название канала
    cell.screen_plan.panel_label = cell.screen_plan.cell;
    cell.screen_plan.panel_label.setTop   (cell.screen_plan.cell.bottom() - 66);
    cell.screen_plan.panel_label.setBottom(cell.screen_plan.cell.bottom() - 9);
    cell.screen_plan.panel_label.setLeft  (cell.screen_plan.cell.left()   + 30);
    cell.screen_plan.panel_label.setRight (cell.screen_plan.cell.right()  - 3);

    // Панель видеоформата
    cell.screen_plan.panel_format_video = cell.screen_plan.cell;
    cell.screen_plan.panel_format_video.setTop   (cell.screen_plan.cell.top()   + 30);
    cell.screen_plan.panel_format_video.setBottom(cell.screen_plan.cell.top()   + 60);
    cell.screen_plan.panel_format_video.setLeft(cell.screen_plan.cell.right() - 130);  // 130 - максимальная ширина для "1080р59,94"
    if((grid == GRID_6x6)||(grid == GRID_12x12))
        cell.screen_plan.panel_format_video.translate( - 50, - 7);
    else
        cell.screen_plan.panel_format_video.translate( - 50, 0);

    // Панель иконок текстовых (ТХТ, и т.д.)
    cell.screen_plan.panel_text_icons = cell.screen_plan.cell;
    cell.screen_plan.panel_text_icons.setTop   (cell.screen_plan.cell.top()   +  30);
    cell.screen_plan.panel_text_icons.setLeft  (cell.screen_plan.cell.left()  +  62);
    cell.screen_plan.panel_text_icons.setRight (cell.screen_plan.cell.left()  + cell.screen_plan.cell.width() / 3);
    cell.screen_plan.panel_text_icons.setBottom(cell.screen_plan.cell.top()   +  70);

    // Панель SCTE 104
    cell.screen_plan.panel_scte_104 = cell.screen_plan.panel_text_icons;
    cell.screen_plan.panel_scte_104.translate(0, cell.screen_plan.panel_text_icons.height());
    cell.screen_plan.panel_scte_104.setBottom(cell.screen_plan.panel_scte_104.bottom() + 11);
    cell.screen_plan.panel_scte_104.setWidth(96);
}

void Layout::draw_label_object(QImage &image, label_cell_t label_cell)
{
    QPainter painter(&image);
    painter.setFont(label_cell.label.font);
    painter.setPen(QPen(Qt::white));

    painter.drawText(label_cell.label.size, Qt::AlignCenter, label_cell.label.text);
    painter.end();

    draw_frame(image, QColor(64,64,64), label_cell.frame, PEN_WIDTH * 2);
}


void Layout::draw_layout_object(QImage &image, layout_object_t layout_object)
{
    if(!solo_mode.enable)
        draw_frame(image, QColor(64,64,64), layout_object.screen_plan.cell, PEN_WIDTH * 2);

    if(layout_object.cell.style){
        get_image_TALLY_indicator_old_style(image, layout_object.screen_old_style.panel_tally_red,   TALLY_RED_OFF_COLOR);
        get_image_TALLY_indicator_old_style(image, layout_object.screen_old_style.panel_tally_green, TALLY_GREEN_OFF_COLOR);
    }

    if(teletext_cell.enable && layout_object.cell.input == 0) return;

    QString label = layout_object.sdi_label;
    if(layout_object.cell.style){
        draw_old_style_text_panel(image, layout_object.screen_old_style.panel_label.cell, layout_object.screen_old_style.panel_label.font, label);
    }
    else{
        draw_transparant_text_panel(image, layout_object.screen_plan.panel_label, FONT_CHANNEL_NAME_SIZE, Qt::AlignCenter, label);
    }

    if(layout_object.cell.sdi_format_display){
        QString sdi_format = mtvsystem->get_sdi_format_str(layout_object.cell.input);
        draw_transparant_text_panel(image, layout_object.screen_plan.panel_format_video, FONT_SDI_FORMAT_SIZE, Qt::AlignRight, sdi_format);
    }

    display_text_icons(image, layout_object.screen_plan.panel_text_icons, layout_object.cell.input);
}

void Layout::set_label_plane()
{
int width, height;

    for(uint i = 0; i < SizeOfArray(label_cell); ++i){

        width  =  width_grid;
        height =  height_grid;

        int pos_x = width  * label_cell[i].x;
        int pos_y = height * label_cell[i].y;

        width  *= label_cell[i].scale_x + 1;
        height *= label_cell[i].scale_y + 1;

        label_cell[i].frame.setRect( pos_x, pos_y, width, height);

        QRect rec;
        rec.setRect(pos_x, pos_y, width, height);

        // Положение Label при произвольном размере окон
        label_cell[i].label.cell = rec;

        // Размер Label
        label_cell[i].label.size = label_cell[i].label.cell;
        int width_label_cell = label_cell[i].label.cell.width() * 0.90;
        label_cell[i].label.size.setWidth(width_label_cell);
        int height_label_cell = label_cell[i].label.cell.height() * 0.80;
        label_cell[i].label.size.setHeight(height_label_cell);

        label_cell[i].label.size.moveCenter(label_cell[i].label.cell.center());

        int FontSize = label_cell[i].label.size.height() * 0.90;
        label_cell[i].label.font.setPointSize(FontSize);
        label_cell[i].label.font.setWeight(QFont::Normal);
        label_cell[i].label.font.setFamily("Roboto");

        QFontMetrics fm(label_cell[i].label.font);
        float factor = (float)label_cell[i].label.size.width() / (float)fm.horizontalAdvance(label_cell[i].label.text);

        if(factor < 1) // уменьшать текст если он шире окна
            label_cell[i].label.font.setPointSize(FontSize * factor);
    }
}


void Layout::set_layout_plane()
{
    for(uint i = 0; i < SizeOfArray(layout_object); ++i){
        set_cell_plane(layout_object[i]);
    }
}


void Layout::clear_alarm(layout_object_t layout_object)
{
    if(!layout_object.screen_plan.enable_video) return;

    QRect cell = layout_object.screen_plan.cell;

    QRect clear_rec = get_alarm_label_rec(cell);
    clear_rec.setHeight(clear_rec.height() * 3.50);
    clear_rec.moveCenter(cell.center());

    QImage image_clear(clear_rec.width(), clear_rec.height(),  QImage::Format_ARGB32);
    image_clear.fill(0);
    blit_to_frame(&image_clear, clear_rec.x(),  clear_rec.y());

    display_scte_104(layout_object.cell.input &0x07); // Обновить метку 104 т.к. на окнах малого размера clear_alarm затирает часть метки
}


QRect Layout::get_alarm_label_rec(QRect cell)
{
const float ratio_19_9 = (float)16 / (float)9;

    QRect alarm_label_rec = cell;

    alarm_label_rec.setHeight(cell.height() * 0.12);
    alarm_label_rec.setWidth(cell.width()  * 0.75);

    float ratio = (float)cell.width() / (float)cell.height();

    if(ratio > ratio_19_9){
        int w = (float)cell.height() * (float)ratio_19_9;
        alarm_label_rec.setWidth(w * 0.75);
    }
    else{
        int h = (float)cell.width() / (float)ratio_19_9;
        alarm_label_rec.setHeight(h * 0.12);
    }

    alarm_label_rec.moveCenter(cell.center());

    return alarm_label_rec;
}

void Layout::draw_alarm_label(int index, layout_object_t layout_object)
{
    if(!layout_object.screen_plan.enable_video) return;
    if(solo_mode.enable && (solo_mode.input != index)) return;

    QList<alarm_t> alarm_label = layout_object.alarm;
    QRect cell = layout_object.screen_plan.cell;

    if(alarm_label.size() == 0) return;

    QRect alarm_label_rec = get_alarm_label_rec(cell);
    alarm_label_rec.translate(0, - alarm_label_rec.height() * (alarm_label.size() - 1) / 2);

    int FontSize = alarm_label_rec.height() * 0.55;

    int gap = alarm_label_rec.height() * 0.09;
    int offset_h = alarm_label_rec.height() + gap;
    int radius = alarm_label_rec.height() * 0.13;

    QImage image_alarm(alarm_label_rec.width(), alarm_label_rec.height(),  QImage::Format_ARGB32);
    QRect alarm_rec;
    alarm_rec = QRect(0, 0, alarm_label_rec.width(), alarm_label_rec.height());
    QPainter painter_alarm(&image_alarm);
    painter_alarm.setFont(QFont("Roboto", FontSize, QFont::Normal));

    for(int i = 0; i < alarm_label.size(); i++){
        image_alarm.fill(0);

        painter_alarm.setBrush(alarm_label.at(i).bkground_color);
        painter_alarm.setPen(QPen(alarm_label.at(i).bkground_color));
        painter_alarm.drawRoundedRect(alarm_rec, radius, radius, Qt::AbsoluteSize);

        int elapsed = alarm_label.at(i).time.elapsed() / 1000;
        QString elapsed_str = sec_to_TimeStr(elapsed);

        painter_alarm.setPen(QPen(Qt::white));
        painter_alarm.drawText(alarm_rec, Qt::AlignCenter, alarm_label.at(i).text + elapsed_str);
        blit_to_frame(&image_alarm, alarm_label_rec.x(),  alarm_label_rec.y());
        mtvsystem->draw_overlay_fast(&image_alarm, alarm_label_rec.x(),  alarm_label_rec.y());
        alarm_label_rec.translate(0, offset_h);
    }
}


void Layout::check_video_loss(int cell_index)
{
int error, error_old;

    int k = cascade.num * 16 + cell_index;

    if(!mtvsystem->get_sdi_status(cell_index))
        error = 1;
    else
        error = 0;

    if(!layout_object[k].cell.enable) error = 0;

    int index = get_alarm_index(layout_object[k], VIDEO_LOST);
    if(index < 0)
        error_old = 0;
    else
        error_old = 1;

    if(error_old == error) return;

    clear_alarm(layout_object[k]);

    if(error){
        alarm_t alarm;
        alarm.bkground_color = QColor(255, 0, 0, 225);
        alarm.text = "Video loss";
        alarm.type = VIDEO_LOST;
        alarm.time.start();
        layout_object[k].alarm.append(alarm);
    }
    else{
        layout_object[k].alarm.removeAt(index);
    }

    if(error)
        eventlog_add_input_state("Video Input %1: Loss", cell_index);

    if(error && (cell_index == 0)){
        op47[0] = 0;
        clean_teletext_image(0); // "0" - телетекст отображается только в первом окне
    }

}
/*---------------------------------------------------------------------------*/
QString Layout::get_num_input(int in_num)
{
QString str;
int cascade_num, input;

    cascade_num = cascade.num + 1;
    input   = in_num + 1;

    if(cascade.mode == Layout::STAND_ALONE)
        str = QString("%1").arg(input);
    else
        str = QString("%1.%2").arg(cascade_num).arg(input);

    return str;
}
/*---------------------------------------------------------------------------*/
void Layout::update_alarm()
{
static int format[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

    for(int i = 0; i < 16; ++i){
        if(format[i] != mtvsystem->get_sdi_format(i)){
            format[i] = mtvsystem->get_sdi_format(i);

            if(mtvsystem->get_sdi_status(i)){
                QString str, state_str;

                if((cascade.mode > STAND_ALONE) && (i == 15) && !cascade.last_slave_device)
                    continue;   // Не писать в журнал формат 16-го входа

                state_str = mtvsystem->get_sdi_format_str(i);;
                eventlog_add_input_state("Video Input %1: " + state_str, i);
            }
        }
    }
}
/*---------------------------------------------------------------------------*/
void Layout::clean_teletext_image(int channel)
{
    if(!teletext_cell.enable) return;
    if(op47[channel]) return;

    QRect panel = teletext_cell.panel_teletext;
    QImage image_teletext(panel.width(), panel.height(), QImage::Format_ARGB32);
    image_teletext.fill(QColor(0, 0, 0, 0));

    int x = panel.x();
    int y = panel.y();

    blit_to_frame(&image_teletext, x, y);
    mtvsystem->draw_overlay_fast(&image_teletext, x, y);
}


void Layout::display_teletext(QImage image_teletext)
{
    if(!teletext_cell.enable) return;

    if(solo_mode.enable && (solo_mode.input != 0)) return;

    QRect panel = teletext_cell.panel_teletext;

    QImage image = fast_scale(image_teletext, panel.width(), panel.height());

    int x = panel.x();
    int y = panel.y();

    blit_to_frame(&image, x, y);
    mtvsystem->draw_overlay_fast(&image, x, y);
    qDebug() << "display_teletext";
    // flush_overlay();
    // mtvsystem->overlay_sync();
}


QImage Layout::fast_scale(QImage image, int width, int height)
{
    QImage ret = QImage(width, height, QImage::Format_ARGB32);

    int ratio_x = image.width()*64 / width;
    int ratio_y = image.height()*64 / height;

    for(int y=0; y<height; y++){
        uint8_t * line_out = ret.scanLine(y);
        uint32_t * pixel_out = (uint32_t*) line_out;

        int new_y = y * ratio_y / 64;
        const uint8_t * line_in = image.constScanLine(new_y);
        const uint32_t * pixel_in = (uint32_t*) line_in;
        for(int x=0; x<width; x++){
            int new_x = x * ratio_x / 64;
            pixel_out[x] = pixel_in[new_x];
        }
    }
    return ret;
}

void Layout::slot_update_op47()
{
static int op47_old[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    for(int i = 0; i < 16; ++i){
        if(op47_latch[i])
            op47[i] = 1;
        else
            op47[i] = 0;

        op47_latch[i] = 0;
    }

    if(op47_old[0] != op47[0]) clean_teletext_image(0); // телетекст отображается только в первом окне

    for(int i = 0; i < 16; ++i){
        if(op47_old[i] != op47[i]){
            op47_old[i] = op47[i];
            display_op47_icons(i);
        }
    }

    qDebug() << "slot_update_op47";
    // flush_overlay();
}


void Layout::scte_104_update()
{
    for(int i = 0; i < 16; ++i){
        display_scte_104(i);
    }
}

void Layout::display_scte_104(int index)
{
    int k = cascade.num * 16 + index;

    if(!layout_object[k].screen_plan.enable_video) return;
    if(!layout_object[k].cell.scte_104_display)    return;
    if(teletext_cell.enable && (k == 0)) return;

    QString text_in  = scte_104_splice[index].in;
    QString text_out = scte_104_splice[index].out;
    QRect panel = layout_object[k].screen_plan.panel_scte_104;

    QImage image_scte_104(panel.width(), panel.height(), QImage::Format_ARGB32);
    image_scte_104.fill(QColor(80, 80, 80, 200));

    QString s = "SCTE-104\n";
    s += "In:    " + text_in + "\n";
    s += "Out: " + text_out;

    int FontSize = 10;

    QPainter painter(&image_scte_104);
    painter.setFont(QFont("Roboto", FontSize, QFont::Normal));

    painter.setBrush(Qt::black);
    QRect rec = QRect(6, 0, panel.width(), panel.height());

    painter.setPen(QPen(Qt::white));
    painter.drawText(rec, Qt::AlignLeft|Qt::AlignVCenter, s);

    painter.end();

    int x = panel.x();
    int y = panel.y();

    blit_to_frame(&image_scte_104, x, y);
    mtvsystem->draw_overlay_fast(&image_scte_104, x, y);
}


void Layout::display_text_icons(QImage &image, QRect panel, int cell_index)
{
    int k = cascade.num * 16 + cell_index;
    if(!layout_object[k].screen_plan.enable_video) return;

    if((k == 0) & teletext_cell.enable) return;

    if(op47[cell_index] && layout_object[k].cell.teletext_icon_display)
        draw_text_icon(image, panel, "TXT");

    /*  Как добавлять новые иконки
        QRect rec = layout_object.screen_plan.panel_text_icons;
        int offset;
        offset = draw_text_icon(image, rec, "TXT");
        rec.setLeft(rec.left() + offset);
        offset = draw_text_icon(image, rec, "CC");
        rec.setLeft(rec.left() + offset);
        offset = draw_text_icon(image, rec, "16:9");
    */
}


void Layout::display_op47_icons(int cell_index)
{
    int k = cascade.num * 16 + cell_index;
    QRect panel = layout_object[k].screen_plan.panel_text_icons;

    QImage image_text_icons(panel.width(), panel.height(), QImage::Format_ARGB32);
    image_text_icons.fill(0x00000000);

    display_text_icons(image_text_icons, QRect(0, 0, panel.width(), panel.height()), cell_index);

    int x =panel.x();
    int y =panel.y();

    blit_to_frame(&image_text_icons, x, y);
    mtvsystem->draw_overlay_fast(&image_text_icons, x, y);
}


void Layout::slot_op47_data(int channel, QByteArray data)
{
    Q_UNUSED(data);
    op47_latch[channel] = 1;
}

void Layout::slot_splice(int index, QString text_in, QString text_out)
{
    scte_104_splice[index].in  = text_in;
    scte_104_splice[index].out = text_out;
    display_scte_104(index);
    qDebug() << "slot_splice";
    // flush_overlay();
}