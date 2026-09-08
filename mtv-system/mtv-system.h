#ifndef PNMPE_SYSTEM_H
#define PNMPE_SYSTEM_H

#include <QtCore/QObject>
#include <QImage>
#include <QTimer>
#include <QTimer>
#include <sys/time.h>
#include "anc-reader.h"
#include <QElapsedTimer>
#include <nlohmann/json.hpp> // jq

#define ANSI_MAGENTA  "\033[35m" //colorised output
#define ANSI_RED  "\033[31m" //colorised output
#define ANSI_RESET  "\033[0m"

typedef struct {
        int id;
        uint32_t interlaced;
        uint32_t width;
        uint32_t height;
} video_format_t;

class PbxMtvSystem : public QObject
{
        Q_OBJECT
public:
        enum {CVO_HDMI, CVO_M26};
        typedef struct {
                int width;
                int height;
                int x;
                int y;
                int enable;
        } image_config_t;
        image_config_t image_config[16];

        void draw_overlay(QImage * image);
        // void draw_overlay(QImage * image, int offset_x, int offset_y);
        void draw_overlay (QImage *img, int x_offset, int y_offset);
        void draw_overlay_fast(QImage *image, int offset_x, int offset_y);
        void overlay_sync();
        void overlay_sync(int source);
        void configure_image(int index, int width, int height, int x, int y, int enable);
        void bars_configure(int index, int x, int x2, int y, int scale, int enable_1, int enable_2);
        int get_sdi_format(int index);
        QString get_sdi_format_str(int index);
        int get_sdi_status(int index);
        int get_sdi_hd(int index);
        QString get_build_id();
        void set_audio_source(int index);
        QList<int> get_audio_level();
        int get_motion(int index);
        void set_dei(int enable);
        void system_set_time(time_t time);
        AncReader * anc_reader;

        PbxMtvSystem();
        ~PbxMtvSystem();
private:
        QTimer sdi_format_timer;
        QTimer sdi_format_notify_timer;
        int sdi_format[16];
        char * buffer;
        int dei;
        QTimer reconfigure_timer;
        void reg_write(uint32_t block, uint32_t addr, uint32_t data);
        uint32_t reg_read(uint32_t block, uint32_t addr);
        void framebuffer_start(int index, int value);
        void framebuffer_reconfigure(int index, int width, int height);
        void mosaic_reconfigure(int index, int x, int y, int width, int height, int enable);
        void scaler_reconfigure(int index, int width_in, int height_in, int width_out, int height_out);
        void scaler_scaler_config(int index, int bypass, int width, int height, 
                int out_width, int out_height, int deinterlace, int unsharp_bypass, int csc_mode);
        void scaler_coeff(int index, uint32_t * coeff);
        void reconfigure();
        QRgb rgb_to_ycrcb(QRgb value);
        QImage * image_to_prpb(QImage * image);
        int limit_color(int value);
        void convert_line(QImage * img, int y, int width, uint8_t * buffer);
        int read_sdi_format(int index);
        void reconfigure_image(int index);
        video_format_t * get_video_format(int id);
        void cvi_configure(int index, int down3g);
        void dei_configure(int enable);
        void cvo_reconfigure(int interlaced, int hdmi_sdi);
        void mosaic_start(int enable, int inrelaced);
        void bars_mute();
        int level_value_to_db(int value);
        void init_overlay_memory();
        QTimer *fps_timer;
        int overlay_fd; // Храним дескриптор открытым для максимальной скорости
        std::string log_path; // for jq
        nlohmann::json log_obj; // for jq
        bool m_jqMode; // Флаг для хранения состояния режима jq
        bool m_rw; // Флаг для хранения состояния режима read write str-mem disabled
        int current_buffer_index;
        int64_t last_elapsed_time = -1; // чтобы не сыпал в терминал
private slots:
        void slot_fps_hardware_trigger();
private Q_SLOTS:
        void sdi_format_timeout();
        void sdi_format_notify_timeout();
        void reconfigure_timeout();

signals:
    void signal_new_format();
};

#endif