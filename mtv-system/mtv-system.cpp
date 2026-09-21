#include <QDebug>
#include <QLoggingCategory>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <arm_neon.h>
#include "mtv-system.h"
#include "str-mem-dev.h"
#include "scaler_coeff.h"
#include <fstream> // for js
#include <QCoreApplication>

#include <cmath> // Для std::abs

//static QLoggingCategory category("SYSTEM");
static QLoggingCategory category("\033[34m MTV-SYSTEM\033[0m"); // ign blue

const char * fname = "/dev/str-mem";
//const int video_size = 1920*1080*3; // RGB
//const int video_size = 1920*1080*2; // YCrCb
//const int video_size = 4149248; // Строго 4149248 байт, как в драйвере! YCrCb
/*для альфа увеличил до 1920 * 1080 * 3 = 6220800*/
const int video_size = 12443648;
#define OVERLAY_IOCTL_FLIP 0x40046D0E
#define MOTION_THR (100)
#define ANCIN ("/dev/tty10")

/*//enum 5161
enum {
                REG_HDMI_OUT,
                REG_MOSAIC,
                REG_FRAMEBUFFER_0,
                REG_FRAMEBUFFER_1,
                REG_FRAMEBUFFER_2,
                REG_FRAMEBUFFER_3,
                REG_FRAMEBUFFER_4,
                REG_FRAMEBUFFER_5,
                REG_FRAMEBUFFER_6,
                REG_FRAMEBUFFER_7,
                REG_FRAMEBUFFER_8,
                REG_FRAMEBUFFER_9,
                REG_FRAMEBUFFER_10,
                REG_FRAMEBUFFER_11,
                REG_FRAMEBUFFER_12,
                REG_FRAMEBUFFER_13,
                REG_FRAMEBUFFER_14,
                REG_FRAMEBUFFER_15,
                REG_SDI_ADAPTER,
                REG_CVI_0,
                REG_SCALER_0,
                REG_CVI_1,
                REG_SCALER_1 ,
                REG_CVI_2,
                REG_SCALER_2,
                REG_CVI_3,
                REG_SCALER_3,
                REG_CVI_4,
                REG_SCALER_4,
                REG_CVI_5,
                REG_SCALER_5,
                REG_CVI_6,
                REG_SCALER_6,
                REG_CVI_7,
                REG_SCALER_7,
                REG_CVI_7a,
                REG_SCALER_7a,
                REG_CVI_8,
                REG_SCALER_8,
                REG_CVI_9,
                REG_SCALER_9,
                REG_CVI_10,
                REG_SCALER_10,
                REG_CVI_11,
                REG_SCALER_11,
                REG_CVI_12,
                REG_SCALER_12,
                REG_CVI_13,
                REG_SCALER_13,
                REG_CVI_14,
                REG_SCALER_14,
                REG_CVI_15,
                REG_SCALER_15,
                REG_BUILDID,
                REG_BARS,
                REG_AUDIO_SELECTOR,
                REG_MOTION_0,
                REG_MOTION_1,
                REG_MOTION_2,
                REG_MOTION_3,
                REG_MOTION_4,
                REG_MOTION_5,
                REG_MOTION_6,
                REG_MOTION_7,
                REG_MOTION_8,
                REG_MOTION_9,
                REG_MOTION_10,
                REG_MOTION_11,
                REG_MOTION_12,
                REG_MOTION_13,
                REG_MOTION_14,
                REG_MOTION_15,
                REG_DEI,
                REG_SDI_CVO, 
                END
};
*/

//enum 508
enum {
        REG_HDMI_OUT,
        REG_MOSAIC,
        REG_FRAMEBUFFER_0,
        REG_FRAMEBUFFER_1,
        REG_FRAMEBUFFER_2,
        REG_FRAMEBUFFER_3,
        REG_FRAMEBUFFER_4,
        REG_FRAMEBUFFER_5,
        REG_FRAMEBUFFER_6,
        REG_FRAMEBUFFER_7,
        REG_FRAMEBUFFER_8,
        REG_FRAMEBUFFER_9,
        REG_FRAMEBUFFER_10,
        REG_FRAMEBUFFER_11,
        REG_FRAMEBUFFER_12,
        REG_FRAMEBUFFER_13,
        REG_FRAMEBUFFER_14,
        REG_FRAMEBUFFER_15,
        REG_SDI_ADAPTER,
        REG_CVI_0,
        REG_SCALER_0,
        REG_CVI_1,
        REG_SCALER_1,
        REG_CVI_2,
        REG_SCALER_2,
        REG_CVI_3,
        REG_SCALER_3,
        REG_CVI_4,
        REG_SCALER_4,
        REG_CVI_5,
        REG_SCALER_5,
        REG_CVI_6,
        REG_SCALER_6,
        REG_CVI_7,
        REG_SCALER_7,
        REG_CVI_8,
        REG_SCALER_8,
        REG_CVI_9,
        REG_SCALER_9,
        REG_CVI_10,
        REG_SCALER_10,
        REG_CVI_11,
        REG_SCALER_11,
        REG_CVI_12,
        REG_SCALER_12,
        REG_CVI_13,
        REG_SCALER_13,
        REG_CVI_14,
        REG_SCALER_14,
        REG_CVI_15,
        REG_SCALER_15,
        REG_BUILDID,
        REG_BARS,
        REG_AUDIO_SELECTOR, 
        REG_MOTION_0,
        REG_MOTION_1,
        REG_MOTION_2,
        REG_MOTION_3,
        REG_MOTION_4,
        REG_MOTION_5,
        REG_MOTION_6,
        REG_MOTION_7,
        REG_MOTION_8,
        REG_MOTION_9,
        REG_MOTION_10,
        REG_MOTION_11,
        REG_MOTION_12,
        REG_MOTION_13,
        REG_MOTION_14,
        REG_MOTION_15,
        REG_DEI,
        REG_SDI_CVO,
};

QList <QString> enumlist = {
        "REG_HDMI_OUT",
        "REG_MOSAIC",
        "REG_FRAMEBUFFER_0",
        "REG_FRAMEBUFFER_1",
        "REG_FRAMEBUFFER_2",
        "REG_FRAMEBUFFER_3",
        "REG_FRAMEBUFFER_4",
        "REG_FRAMEBUFFER_5",
        "REG_FRAMEBUFFER_6",
        "REG_FRAMEBUFFER_7",
        "REG_FRAMEBUFFER_8",
        "REG_FRAMEBUFFER_9",
        "REG_FRAMEBUFFER_10",
        "REG_FRAMEBUFFER_11",
        "REG_FRAMEBUFFER_12",
        "REG_FRAMEBUFFER_13",
        "REG_FRAMEBUFFER_14",
        "REG_FRAMEBUFFER_15",
        "REG_SDI_ADAPTER",
        "REG_CVI_0",
        "REG_SCALER_0",
        "REG_CVI_1",
        "REG_SCALER_1",
        "REG_CVI_2",
        "REG_SCALER_2",
        "REG_CVI_3",
        "REG_SCALER_3",
        "REG_CVI_4",
        "REG_SCALER_4",
        "REG_CVI_5",
        "REG_SCALER_5",
        "REG_CVI_6",
        "REG_SCALER_6",
        "REG_CVI_7",
        "REG_SCALER_7",
        "REG_CVI_8",
        "REG_SCALER_8",
        "REG_CVI_9",
        "REG_SCALER_9",
        "REG_CVI_10",
        "REG_SCALER_10",
        "REG_CVI_11",
        "REG_SCALER_11",
        "REG_CVI_12",
        "REG_SCALER_12",
        "REG_CVI_13",
        "REG_SCALER_13",
        "REG_CVI_14",
        "REG_SCALER_14",
        "REG_CVI_15",
        "REG_SCALER_15",
        "REG_BUILDID",
        "REG_BARS",
        "REG_AUDIO_SELECTOR", 
        "REG_MOTION_0",
        "REG_MOTION_1",
        "REG_MOTION_2",
        "REG_MOTION_3",
        "REG_MOTION_4",
        "REG_MOTION_5",
        "REG_MOTION_6",
        "REG_MOTION_7",
        "REG_MOTION_8",
        "REG_MOTION_9",
        "REG_MOTION_10",
        "REG_MOTION_11",
        "REG_MOTION_12",
        "REG_MOTION_13",
        "REG_MOTION_14",
        "REG_MOTION_15",
        "REG_DEI",
        "REG_SDI_CVO"
};

#define FORMAT_SD (0<<4)
#define FORMAT_HD (1<<4)
#define FORMAT_3G (3<<4)

video_format_t video_format[] = {
        // 625i50
        {
                .id = FORMAT_SD|1,
                .interlaced = 1,
                .width = 720,
                .height = 288,
        },
        // 1080i59.94
        {
                .id = FORMAT_HD|4,
                .interlaced = 1,
                .width = 1920,
                .height = 540,
        },
        // 1080i50
        {
                .id = FORMAT_HD|5,
                .interlaced = 1,
                .width = 1920,
                .height = 540,
        },
        // 720p59.94 -> 720i59.94
        {
                .id = FORMAT_HD|7,
                .interlaced = 0,
                .width = 1280,
                .height = 360,
        },
        // 720p50 -> 720i50
        {
                .id = FORMAT_HD|8,
                .interlaced = 0,
                .width = 1280,
                .height = 360,
        },
        // 1080p59.94 -> 1080i59.94
        {
                .id = FORMAT_3G|12,
                .interlaced = 0,
                .width = 1920,
                .height = 540,
        },
        // 1080p50 -> 1080i50
        {
                .id = FORMAT_3G|13,
                .interlaced = 0,
                .width = 1920,
                .height = 540,
        },
        // 1080p25 -> 1080i25
        {
                .id = FORMAT_HD|13,
                .interlaced = 0,
                .width = 1920,
                .height = 540,
        },
        {
                .id = -1,
                .interlaced = 0,
                .width = 0,
                .height = 0,
        }
};

int32_t level_to_db_table[] = {
        0, -100,
        3, -60,
        7, -59,
        12, -58,
        16, -57,
        21, -56,
        26, -55,
        31, -54,
        35, -53,
        40, -52,
        46, -51,
        51, -50,
        56, -49,
        61, -48,
        67, -47,
        72, -46,
        78, -45,
        84, -44,
        90, -43,
        96, -42,
        102, -41,
        109, -40,
        116, -39,
        123, -38,
        130, -37,
        137, -36,
        145, -35,
        153, -34,
        162, -33,
        171, -32,
        180, -31,
        191, -30,
        201, -29,
        212, -28,
        223, -27,
        233, -26,
        244, -25,
        255, -24,
        265, -23,
        276, -22,
        287, -21,
        297, -20,
        308, -19,
        319, -18,
        329, -17,
        340, -16,
        351, -15,
        361, -14,
        372, -13,
        383, -12,
        393, -11,
        404, -10,
        415, -9,
        425, -8,
        436, -7,
        447, -6,
        457, -5,
        468, -4,
        479, -3,
        489, -2,
        500, -1,
        511, 0,
        //
        -1, -1,
};

typedef struct {
        int id;
        uint32_t h_front_porch;
        uint32_t h_sync;
        uint32_t h_back_porch;
        uint32_t total_line;
        uint32_t sdmux;
        uint32_t interlaced;
        uint32_t v_active;
        uint32_t v_front_porch;
        uint32_t v_sync;
        uint32_t v_2_front_porch;
        uint32_t v_2_sync;
        uint32_t v_2_vsync_pixel;
        uint32_t f_rising;
        uint32_t f_falling;
        uint32_t total_lines;
        uint32_t total_lines_f2;
        uint32_t width;
        uint32_t height;
} cvo_settings_t;

cvo_settings_t cvo_1080i50 = {
        .id = 0,
        .h_front_porch = 528,
        .h_sync = 572,
        .h_back_porch = 720,
        .total_line = 2640,
        .sdmux = 0,
        .interlaced = 1,
        .v_active = 540,
        .v_front_porch = 542,
        .v_sync = 547,
        .v_2_front_porch = 542,
        .v_2_sync = 547,
        .v_2_vsync_pixel = 1848,
        .f_rising = 543,
        .f_falling = 542,
        .total_lines = 563,
        .total_lines_f2 = 562,
        .width = 1920,
        .height = 540,
};

cvo_settings_t cvo_1080p25 = {
        .id = 0,
        .h_front_porch = 528,
        .h_sync = 572,
        .h_back_porch = 720,
        .total_line = 2640,
        .sdmux = 0,
        .interlaced = 0,
        .v_active = 1080,
        .v_front_porch = 1084,
        .v_sync = 1089,
        .v_2_front_porch = 1084,
        .v_2_sync = 1089,
        .v_2_vsync_pixel = 1848,
        .f_rising = 543,
        .f_falling = 542,
        .total_lines = 1125,
        .total_lines_f2 = 1125,
        .width = 1920,
        .height = 1080,
};

PbxMtvSystem::PbxMtvSystem()
{
        // Проверяем аргумент один раз при старте
        m_jqMode = QCoreApplication::arguments().contains("--jq");
        m_rw = QCoreApplication::arguments().contains("--rwdis");
        current_buffer_index = 1;
        
        if (m_jqMode) {
                //qDebug(category) << "jq mode is enabled for PbxMtvSystem";
        
                /*for js*/
                // СТАРТ ПРИЛОЖЕНИЯ: Один раз читаем файл с диска, если он существует
                std::ifstream input_file(log_path);
                if (input_file.is_open()) {
                        try {
                                input_file >> log_obj;
                        } catch (const nlohmann::json::parse_error& e) {
                                log_obj = nlohmann::json::object(); // Если файл битый, создаем пустой {}
                        }
                        input_file.close();
                } else {
                        log_obj = nlohmann::json::object(); // Если файла нет, создаем пустой {}
                }
        }
        dei = 0;
        reconfigure_timer.setSingleShot(true);
        connect(&reconfigure_timer, &QTimer::timeout, this, &PbxMtvSystem::reconfigure_timeout);
        //buffer = (char*) malloc(video_size);
        //buffer = static_cast<char*>(aligned_alloc(64, video_size));
        init_overlay_memory();
        uint8_t* start_address = reinterpret_cast<uint8_t*>(buffer);
        uint8_t* end_address   = start_address + video_size;
        qDebug(category) << "Buffer START address:" << static_cast<void*>(start_address);
        qDebug(category) << "Buffer END   address:" << static_cast<void*>(end_address);
        //qDebug(category) << "Total buffer size:   " << video_size << "bytes";
        
        connect(&sdi_format_timer, &QTimer::timeout, this, &PbxMtvSystem::sdi_format_timeout);
        sdi_format_timer.start(100);
        memset(&image_config, 0, sizeof(image_config));
        sdi_format_notify_timer.setSingleShot(true);
        connect(&sdi_format_notify_timer, &QTimer::timeout, this, &PbxMtvSystem::sdi_format_notify_timeout);
        set_audio_source(0);
        reconfigure();
        qDebug(category) << "ANCIN : " << ANCIN;
        anc_reader = new AncReader(ANCIN, this);
        anc_reader->start();
}

PbxMtvSystem::~PbxMtvSystem()
{
        free(buffer);
        anc_reader->stop();
        delete anc_reader;
}

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

/*init_overlay_memory() добавьте старт таймера:*/
/*void PbxMtvSystem::init_overlay_memory() {
    overlay_fd = open("/dev/mtv-overlay", O_RDWR);
    if (overlay_fd >= 0) {
        //buffer = (char*)mmap(NULL, 1920*1080*3, PROT_READ | PROT_WRITE, MAP_SHARED, overlay_fd, 0);
        buffer = (char*)mmap(NULL, 1920*1080*2, PROT_READ | PROT_WRITE, MAP_SHARED, overlay_fd, 0);

        if (buffer != MAP_FAILED) {
            qDebug(category) << "Success! Kernel memory mapped via mmap";

            // ИНИЦИАЛИЗИРУЕМ АППАРАТНЫЙ ТАЙМЕР НА 60 Гц (16 мс) 
            fps_timer = new QTimer(this);
            connect(fps_timer, &QTimer::timeout, this, &PbxMtvSystem::slot_fps_hardware_trigger);
            fps_timer->setInterval(16); // 16 мс = ~60 кадров в секунду
            fps_timer->start();
        }else{

                qDebug(category) << "else Success";

        }

    }else{
        qDebug(category) << "if (overlay_fd >= 0)";

    }
}
*/

void PbxMtvSystem::init_overlay_memory() {
    // Выставляем полный размер RGB888 кадра
    //video_size = 1920 * 1080 * 3; /* 6 220 800 байт */

    overlay_fd = open("/dev/mtv-overlay", O_RDWR);
    if (overlay_fd >= 0) {
        // Мапим полные 6.2 Мегабайта буфера vmalloc ядра
        buffer = (char*)mmap(NULL, video_size, PROT_READ | PROT_WRITE, MAP_SHARED, overlay_fd, 0);

        if (buffer != MAP_FAILED) {
            qDebug(category) << "mtv-system: Success! Kernel memory mapped via mmap";
            /*
            qDebug(category) << "mtv-system: Total buffer size: " << video_size << "bytes";
            qDebug(category) << "mtv-system: Buffer START address:" << static_cast<void*>(buffer);
            qDebug(category) << "mtv-system: Buffer END address:  " << static_cast<void*>(buffer + video_size);
            */

            // Инициализация FPS-таймера на частоту обновления экрана (~60 Гц)
            fps_timer = new QTimer(this);
            connect(fps_timer, &QTimer::timeout, this, &PbxMtvSystem::slot_fps_hardware_trigger);
            fps_timer->setInterval(16); // 16 миллисекунд
            fps_timer->start();
        } else {
            qCritical(category) << "mtv-system: critical error mmap! Cause:" << strerror(errno);
        }
    } else {
        qCritical(category) << "mtv-system: Can't open /dev/mtv-overlay! Error:" << strerror(errno);
    }
}


/*void PbxMtvSystem::init_overlay_memory() {
    int fd = open("/dev/mtv-overlay", O_RDWR);
    if (fd >= 0) {
        // Теперь компилятор знает все флаги и успешно соберет Zero-Copy маппинг
        buffer = (char*)mmap(NULL, 1920*1080*3, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        
        if (buffer == MAP_FAILED) {
            qCritical(category) << "critical error: mmap of buffers FPGA return MAP_FAILED!";
        } else {
            qDebug(category) << "Success! Kernel memory mapped to Userspace at:" << static_cast<void*>(buffer);
        }
    } else {
        qCritical(category) << "Failed to open /dev/mtv-overlay for mmap! Error:" << strerror(errno);
    }
}*/


/*Реализуйте функцию триггера, которая будет непрерывно кормить FPGA данными:*/
void PbxMtvSystem::slot_fps_hardware_trigger()
{
    if (overlay_fd >= 0) {
        char kick_signal = 1;
        // Отправляем 1 байт. Драйвер мгновенно перевыставит дескриптор кадра в FPGA
        // Благодаря mmap, процессор не тратит время на копирование пикселей
        ::write(overlay_fd, &kick_signal, 1); 
    }
}



void PbxMtvSystem::framebuffer_start(int index, int value)
{
        uint32_t base;
        
        switch(index){
        default:
        case 0:
                base = REG_FRAMEBUFFER_0;
                break;
        case 1:
                base = REG_FRAMEBUFFER_1;
                break;
        case 2:
                base = REG_FRAMEBUFFER_2;
                break;
        case 3:
                base = REG_FRAMEBUFFER_3;
                break;
        case 4:
                base = REG_FRAMEBUFFER_4;
                break;
        case 5:
                base = REG_FRAMEBUFFER_5;
                break;
        case 6:
                base = REG_FRAMEBUFFER_6;
                break;
        case 7:
                base = REG_FRAMEBUFFER_7;
                break;
        case 8:
                base = REG_FRAMEBUFFER_8;
                break;
        case 9:
                base = REG_FRAMEBUFFER_9;
                break;
        case 10:
                base = REG_FRAMEBUFFER_10;
                break;
        case 11:
                base = REG_FRAMEBUFFER_11;
                break;
        case 12:
                base = REG_FRAMEBUFFER_12;
                break;
        case 13:
                base = REG_FRAMEBUFFER_13;
                break;
        case 14:
                base = REG_FRAMEBUFFER_14;
                break;
        case 15:
                base = REG_FRAMEBUFFER_15;
                break;
        }

        if(value)
                reg_write(base, 0, 3);
        else
                reg_write(base, 0, 0);
}

void PbxMtvSystem::framebuffer_reconfigure(int index, int width, int height)
{
        uint32_t base;
        
        switch(index){
        default:
        case 0:
                base = REG_FRAMEBUFFER_0;
                break;
        case 1:
                base = REG_FRAMEBUFFER_1;
                break;
        case 2:
                base = REG_FRAMEBUFFER_2;
                break;
        case 3:
                base = REG_FRAMEBUFFER_3;
                break;
        case 4:
                base = REG_FRAMEBUFFER_4;
                break;
        case 5:
                base = REG_FRAMEBUFFER_5;
                break;
        case 6:
                base = REG_FRAMEBUFFER_6;
                break;
        case 7:
                base = REG_FRAMEBUFFER_7;
                break;
        case 8:
                base = REG_FRAMEBUFFER_8;
                break;
        case 9:
                base = REG_FRAMEBUFFER_9;
                break;
        case 10:
                base = REG_FRAMEBUFFER_10;
                break;
        case 11:
                base = REG_FRAMEBUFFER_11;
                break;
        case 12:
                base = REG_FRAMEBUFFER_12;
                break;
        case 13:
                base = REG_FRAMEBUFFER_13;
                break;
        case 14:
                base = REG_FRAMEBUFFER_14;
                break;
        case 15:
                base = REG_FRAMEBUFFER_15;
                break;
        }
        

        reg_write(base, 1, width*height/3);
        reg_write(base, 2, 
                (dei<<2)|
                (1<<1)|
                (1<<0)
        );
}

void PbxMtvSystem::mosaic_reconfigure(int index, int x, int y, int width, int height, int enable)
{
        uint32_t base;
        
        base = REG_MOSAIC;

        reg_write(base, 16+index*2+0, 
                ((width+x) << 0)
                |((height+y) << 11)
        );
        reg_write(base, 16+index*2+1, 
                ((x) << 0)
                |((y) << 11)
                |((enable)<<22)
        );
}

void PbxMtvSystem::scaler_scaler_config(int index, int bypass, int width, int height, 
        int out_width, int out_height, int deinterlace, int unsharp_bypass, int csc_mode)
{
        int base;

        switch(index){
        default:
        case 0:
                base = REG_SCALER_0;
                break;
        case 1:
                base = REG_SCALER_1;
                break;
        case 2:
                base = REG_SCALER_2;
                break;
        case 3:
                base = REG_SCALER_3;
                break;
        case 4:
                base = REG_SCALER_4;
                break;
        case 5:
                base = REG_SCALER_5;
                break;
        case 6:
                base = REG_SCALER_6;
                break;
        case 7:
                base = REG_SCALER_7;
                break;
        case 8:
                base = REG_SCALER_8;
                break;
        case 9:
                base = REG_SCALER_9;
                break;
        case 10:
                base = REG_SCALER_10;
                break;
        case 11:
                base = REG_SCALER_11;
                break;
        case 12:
                base = REG_SCALER_12;
                break;
        case 13:
                base = REG_SCALER_13;
                break;
        case 14:
                base = REG_SCALER_14;
                break;
        case 15:
                base = REG_SCALER_15;
                break;
        }

        if(out_height==0 || out_width==0)
                return;
        
        uint32_t out_line_inc = (height-6-1)*4096/(out_height);
        uint32_t out_pixel_inc = (width-8-1)*4096/(out_width);

        reg_write(base, 2, out_width  - 1);
        reg_write(base, 3, out_height);
        reg_write(base, 4, out_line_inc);
        reg_write(base, 5, out_pixel_inc);
        // enable
        reg_write(base, 6, 0
                |(unsharp_bypass<<0)
                |(bypass<<1)
                |(csc_mode<<2)
                |(deinterlace<<4)                
        );
        reg_write(base, 0, width - 1);
        reg_write(base, 1, height - 1);
}

void PbxMtvSystem::scaler_coeff(int index, uint32_t * coeff)
{
        int base;
        switch(index){
        default:
        case 0:
                base = REG_SCALER_0;
                break;
        case 1:
                base = REG_SCALER_1;
                break;
        case 2:
                base = REG_SCALER_2;
                break;
        case 3:
                base = REG_SCALER_3;
                break;
        case 4:
                base = REG_SCALER_4;
                break;
        case 5:
                base = REG_SCALER_5;
                break;
        case 6:
                base = REG_SCALER_6;
                break;
        case 7:
                base = REG_SCALER_7;
                break;
        case 8:
                base = REG_SCALER_8;
                break;
        case 9:
                base = REG_SCALER_9;
                break;
        case 10:
                base = REG_SCALER_10;
                break;
        case 11:
                base = REG_SCALER_11;
                break;
        case 12:
                base = REG_SCALER_12;
                break;
        case 13:
                base = REG_SCALER_13;
                break;
        case 14:
                base = REG_SCALER_14;
                break;
        case 15:
                base = REG_SCALER_15;
                break;
        }
        
        for(unsigned int i=0; i<32; i++){
                reg_write(base, 10, coeff[i*3+0]);
                reg_write(base, 11, coeff[i*3+1]);
                reg_write(base, 12, coeff[i*3+2]);
        }
}

void PbxMtvSystem::scaler_reconfigure(int index, int width_in, int height_in, int width_out, int height_out)
{
        int bypass = 0;
        if((width_in == width_out)&&(height_in == height_out))
                bypass = 1;
        scaler_scaler_config(index, bypass, width_in, height_in, width_out, height_out, 0, 0, 0);
        if(width_out>=width_in){
                scaler_coeff(index, coeff_100);
                scaler_coeff(index, coeff_y_100);
        }else{
                if(width_out*100 / width_in >= 75){
                        scaler_coeff(index, coeff_75);
                        scaler_coeff(index, coeff_y_75);
                }else if(width_out*100 / width_in >= 50){
                        scaler_coeff(index, coeff_50);
                        scaler_coeff(index, coeff_y_50);
                }else if(width_out*100 / width_in >= 25){
                        scaler_coeff(index, coeff_25);
                        scaler_coeff(index, coeff_y_25);
                }else{
                        scaler_coeff(index, coeff_10);
                        scaler_coeff(index, coeff_y_10);
                }
        }
}

/*//main void
void PbxMtvSystem::reg_write(uint32_t block, uint32_t addr, uint32_t data)
{
        strmem_reg_data reg_data;
        int ret;
        int fd;

        reg_data.block = block;
        reg_data.address = addr*4;
        reg_data.data = data;
        reg_data.rw = STR_REG_WRITE;

        fd = open(fname, O_RDONLY);
        if(fd<0){
                return;
        }
        ret = ioctl(fd, STRMEM_IOCTL_REG, &reg_data);
	if(ret<0){
	        printf("ioctl error\n");
	}
        close(fd);
}
*/

void PbxMtvSystem::reg_write(uint32_t block, uint32_t addr, uint32_t data)
{
        strmem_reg_data reg_data;
        int ret;
        int fd;

        reg_data.block = block;
        reg_data.address = addr * 4;
        reg_data.data = data;
        reg_data.rw = STR_REG_WRITE;
        if (m_rw){
                fd = open(fname, O_RDONLY);
                if (fd < 0) {
                        return;
                }
                ret = ioctl(fd, STRMEM_IOCTL_REG, &reg_data);
                if (ret < 0) {
                        printf("ioctl error\n");
                }
                close(fd);

        }
        

        // ОБНОВЛЕНИЕ ДАННЫХ В ПАМЯТИ
        QStringList args = QCoreApplication::arguments();

        if (args.contains("--jq")) {
                //qDebug(category) << "jq mode is enabled";
        
                std::string block_key ; // = "block_" + std::to_string(block);
                std::string addr_key  = "addr_0x" + std::to_string(addr * 4);
                //std::string block_list_key; 
                if (block >= 0 && block < enumlist.size()){
                        block_key = enumlist.at(block).toStdString();
                        // Если блока нет в памяти, добавляем его объект
                        if (!log_obj.contains(block_key)) {
                                log_obj[block_key] = nlohmann::json::object();
                        }

                        // Добавляем значение регистра в массив истории этого адреса
                        log_obj[block_key][addr_key].push_back(data);

                        // СБРОС ИЗМЕНЕНИЙ НА ДИСК
                        std::ofstream output_file(log_path);
                        if (output_file.is_open()) {
                                output_file << log_obj.dump(4); // pretty-print с отступом 4 пробела
                                output_file.close();
                        }
                }else{
                        qDebug(category) << "Error in reg_write with block_key block" << block;
                }
        }
}


void PbxMtvSystem::bars_configure(int index, int x, int x2, int y, int scale, int enable_1, int enable_2)
{
        if(dei){
                y = y*2;
                scale = (scale + 1) * 2 - 1;
        }

        reg_write(REG_BARS, index*2+0, 0
                |(x<<0)
                |((y/2)<<11)
                |((scale&0x0f)<<22)
                |(enable_1<<26)
        );

        reg_write(REG_BARS, index*2+1, 0
                |(x2<<0)
                |((y/2)<<11)
                |((scale&0x0f)<<22)
                |(enable_2<<26)
        );
}


uint32_t PbxMtvSystem::reg_read(uint32_t block, uint32_t addr)
{
        strmem_reg_data reg_data;
        int ret;
        int fd;

        reg_data.block = block;
        reg_data.address = addr*4;
        reg_data.data = 0;
        reg_data.rw = STR_REG_READ;

        if (m_rw){
                fd = open(fname, O_RDONLY);
                if(fd<0){
                        return 0;
                }
                ret = ioctl(fd, STRMEM_IOCTL_REG, &reg_data);
                if(ret<0){
                        printf("ioctl error\n");
                }
                close(fd);
        }

        
        return reg_data.data;
}

QString PbxMtvSystem::get_build_id()
{
        uint32_t reg;

        reg = reg_read(REG_BUILDID, 0);
        //qDebug(category) << "reg_read(REG_BUILDID, 0)" << hex << reg;
        //qDebug(category) << "reg_read(REG_BUILDID, 0)" << reg << QString("%1%2%3").arg((reg>>16)&0xFF, 2, 10, QLatin1Char('0')).arg((reg>>8)&0xFF, 2, 10, QLatin1Char('0')).arg((reg>>0)&0xFF, 2, 10, QLatin1Char('0'));
        return QString("%1%2%3").arg((reg>>16)&0xFF, 2, 10, QLatin1Char('0')).arg((reg>>8)&0xFF, 2, 10, QLatin1Char('0')).arg((reg>>0)&0xFF, 2, 10, QLatin1Char('0'));
}

int PbxMtvSystem::limit_color(int value)
{
        if(value > 255)
                value = 255;
        if(value < 0)
                value = 0;
        return value;
}

QRgb PbxMtvSystem::rgb_to_ycrcb(QRgb value)
{
        QRgb ret;

        int32_t y_value;
        int32_t cr_value;
        int32_t cb_value;

        int32_t a_value = (value>>24)&0xFF;
        int32_t r_value = (value>>16)&0xFF;
        int32_t g_value = (value>>8)&0xFF;
        int32_t b_value = (value>>0)&0xFF;

        if(a_value!=0){
                y_value = (54*r_value + 183*g_value + 18*b_value)/256;
                cr_value = (r_value-y_value)*256/(238)/2+128;
                cb_value = (b_value-y_value)*256/(201)/2+128;

                y_value = limit_color(y_value);
                cr_value = limit_color(cr_value);
                cb_value = limit_color(cb_value);
        }else{
                y_value = 127;
                cr_value = 127;
                cb_value = 127;
        }

        ret =  0
                |((a_value&0xFF)<<24)
                |((y_value&0xFF)<<16)
                |((cr_value&0xFF)<<8)
                |((cb_value&0xFF)<<0)
                ;
        return ret;
}

QImage * PbxMtvSystem::image_to_prpb(QImage * image)
{
        QImage * ret = new QImage(image->width(), image->height(), QImage::Format_ARGB32);
        Q_CHECK_PTR(ret);

        for(int y=0; y<image->height(); y++){
                const uint8_t * line_in = image->constScanLine(y);
                const uint32_t * pixel_in = (uint32_t*) line_in;
                uint8_t * line = ret->scanLine(y);
                uint32_t * pixel_out = (uint32_t*) line;
                for(int x=0; x<image->width()/2; x++){
                        QRgb pixel1 = rgb_to_ycrcb(pixel_in[x*2]);
                        QRgb pixel2 = rgb_to_ycrcb(pixel_in[x*2+1]);

                        int32_t cr_value = (((pixel1>>8)&0xFF) + ((pixel2>>8)&0xFF))/2;
                        int32_t cb_value = (((pixel1>>0)&0xFF) + ((pixel2>>0)&0xFF))/2;
                        pixel1 = (pixel1&0xFFFF0000)
                                | (cr_value << 8)
                                | (cb_value << 0);
                        pixel2 = (pixel2&0xFFFF0000)
                                | (cr_value << 8)
                                | (cb_value << 0);
                        pixel_out[x*2] = pixel1;
                        pixel_out[x*2+1] = pixel2;
                }
        }
        return ret;
}

const int16_t uint8_crcb_r_data[] = { 
        -19, 56, -19, 56, -19, 56, -19, 56, 
};

const int16_t uint8_crcb_g_data[] = { 
        -37, -46, -37, -46, -37, -46, -37, -46, 
};

const int16_t uint8_crcb_b_data[] = { 
        56, -9, 56, -9, 56, -9, 56, -9,
};

// void PbxMtvSystem::convert_line(QImage * img, int y, int width, uint8_t * buffer)
// {
//         const uint8_t * line = img->constScanLine(y);

//         for(int x=0; x<width; x++){
//                 uint8x8x3_t ycrcb_data;
                
//                 // вычисление y
//                 uint8x8x4_t rgb_data = vld4_u8(line + x*8*4);
//                 ycrcb_data.val[0] = rgb_data.val[3];
//                 int16x8_t data_y = vmulq_s16(vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[2], vmov_n_u8(0))), vmovq_n_s16(27));
//                 data_y = vmlaq_s16(data_y, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[1], vmov_n_u8(0))), vmovq_n_s16(92));
//                 data_y = vmlaq_s16(data_y, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[0], vmov_n_u8(0))), vmovq_n_s16(9));
//                 ycrcb_data.val[1] = vreinterpret_u8_s8(vshrn_n_s16(data_y, 7));

//                 // вычисление cr
//                 int16x8_t data_cr = vmulq_s16(vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[2], vmov_n_u8(0))), vld1q_s16(uint8_crcb_r_data));
//                 data_cr = vmlaq_s16(data_cr, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[1], vmov_n_u8(0))), vld1q_s16(uint8_crcb_g_data));
//                 data_cr = vmlaq_s16(data_cr, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[0], vmov_n_u8(0))), vld1q_s16(uint8_crcb_b_data));
//                 ycrcb_data.val[2] = vreinterpret_u8_s8(vadd_s8(vshrn_n_s16(data_cr, 7), vmov_n_s8(128)));
//                 vst3_u8(buffer+x*3*8, ycrcb_data);
//         }
// }


void PbxMtvSystem::convert_line(QImage * img, int y, int width, uint8_t * buffer, bool darken, int screen_x_start, int screen_y, QImage * cacheImg) {
    if (y < 0 || y >= img->height()) {
        qCritical() << "CRITICAL ERROR: Requested y line" << y << "is out of QImage bounds";
        return;
    }
    if (width <= 0 || width > img->width() || width > 1920) {
        qCritical() << "CRITICAL ERROR: Invalid width:" << width;
        return;
    }

    uint8_t * dst = buffer;
    const uint8_t * line = img->constScanLine(y);
    if (!line) {
        qCritical() << "CRITICAL ERROR: constScanLine(" << y << ") returned NULL!";
        return;
    }

    // Коэффициенты BT.601.
    // Y считаем БЕЗ ЗНАКА: коэффициенты все положительные и в сумме дают 256,
    // поэтому для яркого пикселя (255,255,255) сумма произведений доходит до
    // 255*256 = 65280 — это переполняет int16 (макс. 32767), но укладывается
    // в uint16 (макс. 65535). Со знаковой арифметикой здесь было переполнение,
    // из-за которого белый текст на видео давал "шумные"/битые пиксели.
    uint16x8_t y_r = vmovq_n_u16(77);
    uint16x8_t y_g = vmovq_n_u16(150);
    uint16x8_t y_b = vmovq_n_u16(29);

    // Cb/Cr коэффициенты знакопеременные, их сумма по модулю не превышает 128,
    // так что максимум |255*128| = 32640 спокойно помещается в int16 —
    // тут знаковая арифметика корректна и переполнения нет.
    int16x8_t cb_r = vmovq_n_s16(-43);
    int16x8_t cb_g = vmovq_n_s16(-85);
    int16x8_t cb_b = vmovq_n_s16(128);
    
    int16x8_t cr_r = vmovq_n_s16(128);
    int16x8_t cr_g = vmovq_n_s16(-107);
    int16x8_t cr_b = vmovq_n_s16(-21);

    // Округляем основную сетку обработки вниз до ближайшего кратного 8
    int x = 0;
    int vector_width = width & ~7; 

    darken_area_t dark_zone;
    // Проверяем попадание строки в зону плашки по Y
    bool is_y_inside_dark_zone = (screen_y >= dark_zone.dark_top && screen_y < dark_zone.dark_bottom);

    // Получаем строку для записи в кэш-картинку Layout, если мы в режиме отрисовки виджетов
    uint8_t * cache_line = nullptr;
    if (!darken && mess_exist && is_y_inside_dark_zone && cacheImg && !cacheImg->isNull()) {
        int cache_y = screen_y - dark_zone.dark_top;
        cache_line = cacheImg->scanLine(cache_y); // Открываем строку кэша на запись
    }

    const uint16_t x_offsets[] = {0, 1, 2, 3, 4, 5, 6, 7};
    uint16x8_t v_x_offsets = vld1q_u16(x_offsets);
    uint16x8_t v_dark_left = vmovq_n_u16(dark_zone.dark_left);
    uint16x8_t v_dark_right = vmovq_n_u16(dark_zone.dark_right);

    int dark_zone_width = dark_zone.dark_right - dark_zone.dark_left;

    // Основной цикл: обрабатываем по 8 пикселей за раз
    for(; x < vector_width; x += 8) {
        uint8x8x4_t rgb_data = vld4_u8(line + x * 4);
        
        uint8x8_t b_val = rgb_data.val[0];
        uint8x8_t g_val = rgb_data.val[1];
        uint8x8_t r_val = rgb_data.val[2];
        uint8x8_t a_val = rgb_data.val[3];

        uint16x8_t r_u = vmovl_u8(r_val);
        uint16x8_t g_u = vmovl_u8(g_val);
        uint16x8_t b_u = vmovl_u8(b_val);

        // Y: беззнаковая арифметика (см. комментарий у коэффициентов выше)
        uint16x8_t y_acc = vmulq_u16(r_u, y_r);
        y_acc = vmlaq_u16(y_acc, g_u, y_g);
        y_acc = vmlaq_u16(y_acc, b_u, y_b);
        uint8x8_t y_val = vqshrn_n_u16(y_acc, 8); // насыщающий сдвиг+сужение до 8 бит

        // Cb/Cr: знаковая арифметика (коэффициенты знакопеременные)
        int16x8_t b = vreinterpretq_s16_u16(b_u);
        int16x8_t g = vreinterpretq_s16_u16(g_u);
        int16x8_t r = vreinterpretq_s16_u16(r_u);

        int16x8_t cb_acc = vmulq_s16(r, cb_r);
        cb_acc = vmlaq_s16(cb_acc, g, cb_g);
        cb_acc = vmlaq_s16(cb_acc, b, cb_b);
        uint8x8_t cb_val = vqmovun_s16(vaddq_s16(vshrq_n_s16(cb_acc, 8), vmovq_n_s16(128)));

        int16x8_t cr_acc = vmulq_s16(r, cr_r);
        cr_acc = vmlaq_s16(cr_acc, g, cr_g);
        cr_acc = vmlaq_s16(cr_acc, b, cr_b);
        uint8x8_t cr_val = vqmovun_s16(vaddq_s16(vshrq_n_s16(cr_acc, 8), vmovq_n_s16(128)));

        // Субдискретизация 4:2:2 с усреднением соседних пикселей по горизонтали
        uint8x8x2_t cb_split = vuzp_u8(cb_val, cb_val);
        uint8x8x2_t cr_split = vuzp_u8(cr_val, cr_val);
        uint8x8_t cb_down = vrhadd_u8(cb_split.val[0], cb_split.val[1]); // 4 усреднённых значения Cb
        uint8x8_t cr_down = vrhadd_u8(cr_split.val[0], cr_split.val[1]); // 4 усреднённых значения Cr

        // Собираем итоговую тройку векторов для vst3_u8, дающую при записи
        // порядок байт Cb0 Y0 A0 Cr0 Y1 A1 Cb1 Y2 A2 Cr1 Y3 A3 ...
        uint8x8x3_t out_p1;
        out_p1.val[0] = vzip_u8(cb_down, cr_down).val[0]; // Cb0, Cr0, Cb1, Cr1...
        out_p1.val[1] = y_val;                            // Y0,  Y1,  Y2,  Y3...
        // Управление прозрачностью: Если флаг darken равен true, 
        // альфа-канал принудительно забивается непрозрачным цветом vmov_n_u8(255)
        out_p1.val[2] = darken ? vmov_n_u8(255) : a_val;                            // A0,  A1,  A2,  A3...

        if (darken || !mess_exist || !is_y_inside_dark_zone) vst3_u8(dst, out_p1);
        dst += 24;

        // НАКОПЛЕНИЕ КЭША
        // Если строка пересекается с затемненной плашкой и передан cacheImg, код на лету формирует кэш-картинку:
        if (cache_line) {
                // Векторная маска (m_inside): С помощью vcgeq_u16 (больше или равно) и vcltq_u16 (меньше) 
                // вычисляются глобальные координаты X для каждого из 8 пикселей. 
                // Проверяется, какие из них попадают внутрь dark_left и dark_right
                uint16x8_t v_abs_x = vaddq_u16(vmovq_n_u16(screen_x_start + x), v_x_offsets);
                uint16x8_t m_left = vcgeq_u16(v_abs_x, v_dark_left);
                uint16x8_t m_right = vcltq_u16(v_abs_x, v_dark_right);
                uint16x8_t m_inside = vandq_u16(m_left, m_right);
                uint8x8_t m_inside_u8 = vmovn_u16(m_inside);

                // Затемнение пикселей: Операция vshrn_n_u16(r_u, 1) сдвигает биты вправо на 1. 
                // В бинарной математике это быстрое деление на 2. 
                // То есть цвета RGB исходного изображения становятся в два раза темнее.
                uint8x8_t r_half = vshrn_n_u16(r_u, 1);
                uint8x8_t g_half = vshrn_n_u16(g_u, 1);
                uint8x8_t b_half = vshrn_n_u16(b_u, 1);

                int cache_x = (screen_x_start + x) - dark_zone.dark_left;
                if (cache_x >= 0 && (cache_x + 8) <= dark_zone_width) {
                        uint8x8x4_t current_cache = vld4_u8(cache_line + cache_x * 4);
                        uint8x8x4_t out_cache;
                        // Битовый выбор (vbsl_u8): Инструкция Vector Bitwise Select на основе ранее созданной маски 
                        // решает:
                        // Если пиксель внутри зоны — записать в кэш деленный пополам (затемненный) цвет.
                        // Если пиксель вне зоны — сохранить старый пиксель из кэша (current_cache).
                        out_cache.val[2] = vbsl_u8(m_inside_u8, r_half, current_cache.val[2]); 
                        out_cache.val[1] = vbsl_u8(m_inside_u8, g_half, current_cache.val[1]); 
                        out_cache.val[0] = vbsl_u8(m_inside_u8, b_half, current_cache.val[0]); 
                        out_cache.val[3] = vmov_n_u8(255); 

                        // Измененные пиксели сохраняются обратно в структуру cacheImg.
                        vst4_u8(cache_line + cache_x * 4, out_cache);
                }
        }
    }

    // Хвостовой цикл: обрабатываем оставшиеся пиксели (от 0 до 7 штук) скалярно
    // Это полностью решает проблему с нечетной шириной (например, 131)
        // Хвостовой цикл: обрабатываем оставшиеся пиксели (от 0 до 7 штук) скалярно
    for(; x < width; x += 2) {
        // --- Случай "висящего" последнего нечётного пикселя ---
        if (x == width - 1) {
            const uint8_t * p = line + x * 4;
            uint8_t b = p[0];
            uint8_t g = p[1];
            uint8_t r = p[2];
            uint8_t a = darken ? 255 : p[3];

            int y_val  = (77 * r + 150 * g + 29 * b) >> 8;
            int cb_val = (((-43 * r - 85 * g + 128 * b) >> 8) + 128);
            int cr_val = (((128 * r - 107 * g - 21 * b) >> 8) + 128);

            if (darken || !mess_exist || !is_y_inside_dark_zone) {
                *dst++ = (uint8_t)cb_val;
                *dst++ = (uint8_t)y_val;
                *dst++ = a;
                *dst++ = (uint8_t)cr_val;
                *dst++ = (uint8_t)y_val;
                *dst++ = a;
            } else {
                dst += 6; // пропускаем запись, но сдвигаем dst как в векторном цикле
            }

            // Кэш для одиночного хвостового пикселя
            if (cache_line) {
                int abs_x = screen_x_start + x;
                if (abs_x >= dark_zone.dark_left && abs_x < dark_zone.dark_right) {
                    int cache_x = abs_x - dark_zone.dark_left;
                    if (cache_x >= 0 && cache_x < dark_zone_width) {
                        uint8_t * cp = cache_line + cache_x * 4;
                        cp[2] = r >> 1;
                        cp[1] = g >> 1;
                        cp[0] = b >> 1;
                        cp[3] = 255;
                    }
                }
            }
            break;
        }

        // --- Обычная пара пикселей ---
        const uint8_t * p0 = line + x * 4;
        const uint8_t * p1 = line + (x + 1) * 4;

        int y0  = (77 * p0[2] + 150 * p0[1] + 29 * p0[0]) >> 8;
        int cb0 = ((-43 * p0[2] - 85 * p0[1] + 128 * p0[0]) >> 8) + 128;
        int cr0 = ((128 * p0[2] - 107 * p0[1] - 21 * p0[0]) >> 8) + 128;

        int y1  = (77 * p1[2] + 150 * p1[1] + 29 * p1[0]) >> 8;
        int cb1 = ((-43 * p1[2] - 85 * p1[1] + 128 * p1[0]) >> 8) + 128;
        int cr1 = ((128 * p1[2] - 107 * p1[1] - 21 * p1[0]) >> 8) + 128;

        uint8_t cb_avg = (uint8_t)((cb0 + cb1 + 1) >> 1);
        uint8_t cr_avg = (uint8_t)((cr0 + cr1 + 1) >> 1);

        uint8_t a0 = darken ? 255 : p0[3];
        uint8_t a1 = darken ? 255 : p1[3];

        if (darken || !mess_exist || !is_y_inside_dark_zone) {
            *dst++ = cb_avg;
            *dst++ = (uint8_t)y0;
            *dst++ = a0;

            *dst++ = cr_avg;
            *dst++ = (uint8_t)y1;
            *dst++ = a1;
        } else {
            dst += 6;
        }

        // Кэш для пары пикселей
        if (cache_line) {
            for (int k = 0; k < 2; ++k) {
                const uint8_t * p = (k == 0) ? p0 : p1;
                int abs_x = screen_x_start + x + k;
                if (abs_x >= dark_zone.dark_left && abs_x < dark_zone.dark_right) {
                    int cache_x = abs_x - dark_zone.dark_left;
                    if (cache_x >= 0 && cache_x < dark_zone_width) {
                        uint8_t * cp = cache_line + cache_x * 4;
                        cp[2] = p[2] >> 1; // r_half
                        cp[1] = p[1] >> 1; // g_half
                        cp[0] = p[0] >> 1; // b_half
                        cp[3] = 255;
                    }
                }
            }
        }
    }
}


// const int FRAME_WIDTH = 1920;
// const int FRAME_HEIGHT = 1080;
// const int STRIDE = 1920 * 3;
// const size_t FRAME_SIZE = 6221824;

// struct MacroPixel {
//     unsigned char cb, y0, alpha0, cr, y1, alpha1;
// };

// void PbxMtvSystem::draw_overlay(QImage *img, int x_offset, int y_offset) {
// //     if (!m_mmap_base || img.isNull()) return;

//     // Определяем текущий скрытый буфер (Back-Buffer) для записи кадра
//     int next_write_index = (this->current_buffer_index == 0) ? 1 : 0;
//     // int nextWriteIndex = 0;
// //     unsigned char* active_fb_ptr = m_mmap_base + (nextWriteIndex * FRAME_SIZE);
//     uint8_t* active_fb_ptr = reinterpret_cast<uint8_t*>(buffer + (next_write_index * (video_size / 2)));

//     int img_w = img->width();
//     int img_h = img->height();
//     int macro_screen_width = FRAME_WIDTH / 2;

//     // Выставляем альфу: для объекта 5 (зеленый таймер) полупрозрачность, для остальных 100%
// //     unsigned char custom_alpha = (object_id == 5) ? 64 : 255;
//     unsigned char custom_alpha = 255;

//     // Выравниваем координату X по сетке макропикселей (шаг 2 пикселя)
//     int start_macro_x = (x_offset / 2) * 2;

//     // ------------------------------------------------------------------
//     // ОПТИМИЗИРОВАННАЯ ОТРИСОВКА (БЕЗ ЛИШНЕГО СТИРАНИЯ И МАССИВОВ КООРДИНАТ)
//     // ------------------------------------------------------------------
//     for (int src_y = 0; src_y < img_h; ++src_y) {
//         int dst_y = y_offset + src_y;
//         if (dst_y < 0 || dst_y >= FRAME_HEIGHT) continue; // Защита по Y

//         const unsigned char* rgb_row = img->constScanLine(src_y);
//         MacroPixel* dma_row = (MacroPixel*)(active_fb_ptr + (dst_y * STRIDE));

//         for (int src_x = 0; src_x < img_w; src_x += 2) {
//             int dst_x = start_macro_x + src_x;
//             if (dst_x < 0 || dst_x >= FRAME_WIDTH - 1) continue; // Защита по X

//             int macro_idx = (dst_x / 2) % macro_screen_width;

//             // 1. Мы достаем чистые RGB байты из памяти Qt для пары пикселей
//             int idx0 = src_x * 3;
//             unsigned char r0 = rgb_row[idx0 + 0], g0 = rgb_row[idx0 + 1], b0 = rgb_row[idx0 + 2];
            
//             int idx1 = (src_x + 1) * 3;
//             unsigned char r1 = rgb_row[idx1 + 0], g1 = rgb_row[idx1 + 1], b1 = rgb_row[idx1 + 2];

//             // 2. МАТЕМАТИЧЕСКАЯ ФОРМУЛА КОНВЕРТАЦИИ (Стандарт BT.601)
//             unsigned char y0 = (unsigned char)((77 * r0 + 150 * g0 + 29 * b0) >> 8);
//             unsigned char y1 = (unsigned char)((77 * r1 + 150 * g1 + 29 * b1) >> 8);

//             int r_avg = (r0 + r1) >> 1;
//             int g_avg = (g0 + g1) >> 1;
//             int b_avg = (b0 + b1) >> 1;

//             unsigned char cb = (unsigned char)(((-43 * r_avg - 85 * g_avg + 128 * b_avg) >> 8) + 128);
//             unsigned char cr = (unsigned char)(((128 * r_avg - 107 * g_avg - 21 * b_avg) >> 8) + 128);

//             /*Эта математика берет компьютерные значения RGB и пересчитывает их в 
//             телевизионный стандарт YUV (YCbCr):Y (y0, y1) — это яркость пикселей.cb — это цветоразностный 
//             сигнал синего цвета (показывает, насколько цвет далек от синего).cr — это цветоразностный 
//             сигнал красного цвета.*/

//             // Записываем макропиксель напрямую в DDR ПЛИС Arria 10
//             dma_row[macro_idx] = { cb, y0, custom_alpha, cr, y1, custom_alpha };
//         }
//     }

//     this->current_buffer_index = next_write_index;
//     int ioctl_buffer_idx = this->current_buffer_index;
//     int result = ioctl(this->overlay_fd, 0x40046D0E, &ioctl_buffer_idx);
//     if (result < 0) {
//             qCritical() << "Failed to execute IOCTL FLIP! Error code:" << errno;
//             return;
//     }
// }



// const int FRAME_WIDTH = 1920; 
// const int FRAME_HEIGHT = 1080; 
// const int STRIDE = 1920 * 3; // 1920 пикселей * 3 байта (MacroPixel занимает 6 байт на 2 пикселя)
// const size_t FRAME_SIZE = 6221824; 

// struct MacroPixel { 
//     unsigned char cb, y0, alpha0, cr, y1, alpha1; 
// } __attribute__((packed)); // Гарантируем отсутствие дыр (компиляторного выравнивания)

// void PbxMtvSystem::draw_overlay(QImage *img, int x_offset, int y_offset) {
//     if (!img || img->isNull()) return;

//     // 1. Переключение буферов
//     int next_write_index = (this->current_buffer_index == 0) ? 1 : 0; 
//     // Внимание: проверьте правильность формулы размера буфера (video_size / 2) из вашего оригинального кода
//     uint8_t* active_fb_ptr = reinterpret_cast<uint8_t*>(buffer + (next_write_index * (video_size / 2))); 

//     int img_w = img->width();
//     int img_h = img->height();

//     // Выравниваем начальную координату на экране по сетке макропикселей (шаг 2 пикселя) в меньшую сторону
//     int start_dst_x = (x_offset / 2) * 2; 
//     unsigned char custom_alpha = 255;

//     // Построчная отрисовка
// for (int src_y = 0; src_y < img_h; ++src_y) {
//     int dst_y = y_offset + src_y;
//     if (dst_y < 0 || dst_y >= FRAME_HEIGHT) continue; // Защита по Y

//     // !!! Кастим строку к uint32_t* для работы с целыми пикселями Format_ARGB32 !!!
//     const uint32_t* rgb_row = reinterpret_cast<const uint32_t*>(img->constScanLine(src_y));
//     MacroPixel* dma_row = reinterpret_cast<MacroPixel*>(active_fb_ptr + (dst_y * STRIDE));

//     // Шагаем по оверлею по 2 пикселя
//     for (int src_x = 0; src_x < img_w; src_x += 2) {
//         int dst_x = start_dst_x + src_x;
        
//         // Защита по X
//         if (dst_x < 0 || dst_x >= FRAME_WIDTH - 1) continue; 

//         int macro_idx = dst_x / 2; 

//         // 1. Извлекаем цвета первого пикселя (через макросы Qt — это быстро и безопасно)
//         uint32_t pixel0 = rgb_row[src_x];
//         unsigned char r0 = qRed(pixel0);
//         unsigned char g0 = qGreen(pixel0);
//         unsigned char b0 = qBlue(pixel0);

//         // 2. Извлекаем цвета второго пикселя (с защитой от нечетной ширины)
//         unsigned char r1 = r0, g1 = g0, b1 = b0;
//         if (src_x + 1 < img_w) {
//             uint32_t pixel1 = rgb_row[src_x + 1];
//             r1 = qRed(pixel1);
//             g1 = qGreen(pixel1);
//             b1 = qBlue(pixel1);
//         }

//         // 3. Конвертация RGB -> YUV (BT.601)
//         unsigned char y0 = static_cast<unsigned char>((77 * r0 + 150 * g0 + 29 * b0) >> 8);
//         unsigned char y1 = static_cast<unsigned char>((77 * r1 + 150 * g1 + 29 * b1) >> 8);

//         int r_avg = (r0 + r1) / 2;
//         int g_avg = (g0 + g1) / 2;
//         int b_avg = (b0 + b1) / 2;

//         int cb_val = ((-43 * r_avg - 85 * g_avg + 128 * b_avg) >> 8) + 128;
//         int cr_val = ((128 * r_avg - 107 * g_avg - 21 * b_avg) >> 8) + 128;

//         unsigned char cb = static_cast<unsigned char>(cb_val < 0 ? 0 : (cb_val > 255 ? 255 : cb_val));
//         unsigned char cr = static_cast<unsigned char>(cr_val < 0 ? 0 : (cr_val > 255 ? 255 : cr_val));

//         // 4. Запись в DDR ПЛИС
//         dma_row[macro_idx] = { cb, y0, custom_alpha, cr, y1, custom_alpha };
//     }
// }


//     // 5. Сигнал ПЛИС на переключение кадра (Flip)
//     this->current_buffer_index = next_write_index;
//     int ioctl_buffer_idx = this->current_buffer_index;
//     int result = ioctl(this->overlay_fd, 0x40046D0E, &ioctl_buffer_idx);
//     if (result < 0) {
//         qCritical() << "Failed to execute IOCTL FLIP! Error code:" << errno;
//         return;
//     }
// }




//anton ver
void PbxMtvSystem::draw_overlay(QImage *image, int offset_x, int offset_y)
{
        // ... (Your standard null-pointer checks and boundary checks remain here) ...
        // 1. ЗАЩИТА ОТ NULL-УКАЗАТЕЛЕЙ
        QMutexLocker locker(&m_mutex_draw); 
        if (!image) {
                qCritical() << "CRITICAL ERROR: QImage pointer is NULL!";
                return;
        }
        if (!buffer) {
                qCritical() << "CRITICAL ERROR: Output buffer pointer is NULL!";
                return;
        }

        // 3. ПРОВЕРКА СТРУКТУРЫ И ФОРМАТА QIMAGE
        if (image->format() != QImage::Format_ARGB32 && image->format() != QImage::Format_RGB32) {
                qCritical() << "WARNING: QImage format is not ARGB32/RGB32! Current format:" << image->format();
        }

        Q_ASSERT(image->width() + offset_x <= 1920);
        Q_ASSERT(image->height() + offset_y <= 1080);

        

        // НОВЫЙ СТРАЙД: 1920 пикселей * 3 байта = 5760 байт на строку
        int row_stride = 1920 * 3; 

        int img_w = image->width();  
        int img_h = image->height();
        int aligned_offset_x = (offset_x / 2) * 2;

        if ((offset_x + img_w) > 1920 || (offset_y + img_h) > 1080) {
                qCritical() << "CRITICAL ERROR: Image with offsets goes out of Full HD bounds!";
                return;
        }

    // 1. Calculate next write buffer index using the class variable
    int next_write_index = (this->current_buffer_index == 0) ? 1 : 0;
    uint8_t* start_address = reinterpret_cast<uint8_t*>(buffer + (next_write_index * (video_size / 2)));
//     qDebug(category) << start_address << hex;

    // 2. Render loop using ARM-NEON convert_line
    for (int y = 0; y < image->height(); ++y) {
        int screen_y = y + offset_y;
        uint8_t * current_row_with_offset = start_address + (screen_y * row_stride) + (aligned_offset_x * 3);
        
        convert_line(image, y, image->width(), current_row_with_offset, false, aligned_offset_x, screen_y, m_msgImageCache);
              

    }

    // Переключаем активный индекс и делаем физический ioctl flip
    this->current_buffer_index = next_write_index;
    int ioctl_buffer_idx = this->current_buffer_index;
    int result = ioctl(this->overlay_fd, 0x40046D0E, &ioctl_buffer_idx);
    if (result < 0) {
        qCritical() << "Failed to execute IOCTL FLIP! Error code:" << errno;
        return;
    }
//     qDebug(category) << "current_idx" << this->current_buffer_index;
}

// Быстрый путь для мелких, часто меняющихся элементов (секундная стрелка,
// десятые доли секунды таймера и т.п.).
//
// В отличие от draw_overlay(): пишет НЕ во второй (неактивный) буфер с
// последующим flip, а СРАЗУ в буфер, который FPGA читает ПРЯМО СЕЙЧАС
// (current_buffer_index). Никакого ioctl FLIP не делается - он тут не нужен:
// slot_fps_hardware_trigger() и так "кикает" драйвер каждые ~16мс, повторно
// выставляя дескриптор ТОГО ЖЕ буфера, так что запись подхватится сама
// в пределах одного кадра.
//
// Компромисс: т.к. буфер активный (его в этот самый момент может читать
// FPGA), теоретически возможен micro-artefact/разрыв на текущей строке
// в течение одного кадра, если запись совпадёт по времени с чтением этой
// же строки. Для маленького, редко (относительно 60 Гц) обновляемого
// элемента (клок раз в 100мс) это не должно быть заметно, а на следующем
// кадре всё уже консистентно. Если это станет проблемой - придётся уходить
// на shadow-copy back-buffer (см. обсуждение).
void PbxMtvSystem::draw_overlay_fast(QImage *image, int offset_x, int offset_y, bool darken)
{
    QMutexLocker locker(&m_mutex_draw_fast); 
    QElapsedTimer timer;
    timer.start();  

    if (!image) {
        qCritical() << "CRITICAL ERROR: QImage pointer is NULL!";
        return;
    }
    if (!buffer) {
        qCritical() << "CRITICAL ERROR: Output buffer pointer is NULL!";
        return;
    }
    if (image->format() != QImage::Format_ARGB32 && image->format() != QImage::Format_RGB32) {
        qCritical() << "WARNING: QImage format is not ARGB32/RGB32! Current format:" << image->format();
    }

    // Сохраняем или сбрасываем кэш ОДИН РАЗ до цикла строк, чтобы сберечь такты CPU
    if (darken) {
        m_msgImageCache = image; 
    } else if (!this->mess_exist) {
        m_msgImageCache = nullptr; 
    }

    // Выравниваем offset_x вниз до чётного - сетка макропикселей Cb/Y/A/Cr/Y/A
    // завязана на пары колонок, и convert_line всегда начинает писать с фазы Cb.
    int aligned_offset_x = (offset_x / 2) * 2;

    Q_ASSERT(image->width() + aligned_offset_x <= 1920);
    Q_ASSERT(image->height() + offset_y <= 1080);

    int row_stride = 1920 * 3;
    int img_w = image->width();
    int img_h = image->height();

    if ((aligned_offset_x + img_w) > 1920 || (offset_y + img_h) > 1080) {
        qCritical() << "CRITICAL ERROR: Image with offsets goes out of Full HD bounds!";
        return;
    }

    // Ключевое отличие от draw_overlay(): берём current_buffer_index
    // (активный, читаемый FPGA буфер), а не next_write_index.
    uint8_t* start_address = reinterpret_cast<uint8_t*>(buffer + (this->current_buffer_index * (video_size / 2)));

    for (int y = 0; y < img_h; ++y) {
        int screen_y = y + offset_y;
        // aligned_offset_x = 0;
        uint8_t * current_row_with_offset = start_address + (screen_y * row_stride) + (aligned_offset_x * 3);

        convert_line(image, y, img_w, current_row_with_offset, darken, aligned_offset_x, screen_y, m_msgImageCache);
       
    }
    
    

//     qDebug(category) << "mtvsystem->draw_overlay_fast();" << timer.elapsed() << "milliseconds" << "current_idx" << this->current_buffer_index;
    int64_t current_elapsed = timer.elapsed();

        if (current_elapsed != 0) {
        int64_t current_delta = current_elapsed - last_elapsed_time;
        static int64_t last_delta = 0; 
        
        // Вводим порог чувствительности (например, 5 мс). 
        // Всё, что прыгает меньше этого значения, лог будет игнорировать.
        int64_t delta_change = std::abs(current_delta - last_delta);
        
        // Логируем только если разница между дельтами СУЩЕСТВЕННА (например, > 5 мс)
        // ИЛИ если само время выполнения стало аномально большим (например, > 15 мс)
        if ((delta_change > 15 && last_elapsed_time != 0) || current_elapsed > 15) {
                
                qCDebug(category) << ANSI_MAGENTA << "draw_overlay_fast();" << ANSI_RESET 
                                << "Elapsed:" << current_elapsed << "ms,"
                                << "Delta:" << current_delta << "ms";       
                
                last_delta = current_delta;
        }
        
        last_elapsed_time = current_elapsed;

        }

    // ioctl FLIP осознанно не делаем - buffer index не меняем.
}



void PbxMtvSystem::overlay_sync(){

        overlay_sync(0);
}

/*void PbxMtvSystem::overlay_sync(int source)
{
        QElapsedTimer timer;
        if(source ==1){               
                timer.start();
                qDebug(category) << "====== Start Measuring  overlay_sync==========";
        }
        
        
        int f = open("/dev/mtv-overlay", O_WRONLY);
        if (f < 0) {
                qCritical(category) << "can't open /dev/mtv-overlay! error:" << strerror(errno);
                return; // Вместо падения по Q_ASSERT даем программе шанс продолжить работу
        }

        // Системный вызов write передает данные из вашего выровненного буфера в драйвер ядра.
        // Драйвер pfrt,overlay внутри себя задействует DMA контроллер на шине 0xFF202880.
        ssize_t bytes_written = write(f, buffer, video_size);
        
        if (bytes_written < 0) {
                qCritical(category) << "error write  DMA overlay! error:" << strerror(errno);
        } else if ((size_t)bytes_written != video_size) {
                qWarning(category) << "Insufficient number of bytes written:" << bytes_written << "из" << video_size;
        }

        if(source == 1){
                qDebug(category) << "\t\tit's analog clock:" << source << "writen" << bytes_written;
                qDebug(category) << "overlay_sync operation took" << timer.elapsed() << "milliseconds";
        }


        //int f;

        //Q_CHECK_PTR(buffer);
        //f = open("/dev/mtv-overlay", O_WRONLY);
        //Q_ASSERT(f>0);

        //write(f, buffer, video_size);

        //close(f);
}
*/



void PbxMtvSystem::overlay_sync(int source) {
    // МЫ НИЧЕГО НЕ ДЕЛАЕМ! 
    // Память замаплена, а драйвер крутит прерывания в фоне.
    // Изменения секундной стрелки мгновенно отображаются на мониторе.

        // open() и write() ОТСЮДА ПОЛНОСТЬСТЬЮ УДАЛЕНЫ!
    // Вы просто в фоне перерисовываете пиксели стрелок часов внутри buffer.
    // Высокоскоростной slot_fps_hardware_trigger сам подхватит эти изменения 
    // и выведет их на монитор в пределах 16 миллисекунд.
}


/*void PbxMtvSystem::overlay_sync(int source) {
    int f = open("/dev/mtv-overlay", O_WRONLY);
    if (f >= 0) {
        close(f); // Открытие и закрытие файла запускает DMA в mtv_open драйвера
    }
    if(source == 1){
                qDebug(category) << "\t\tit's analog clock:" << source << "writen";
               
        }
}*/

void PbxMtvSystem::configure_image(int index, int width, int height, int x, int y, int enable)
{
        image_config[index].width = width;
        image_config[index].height = height / 2;
        image_config[index].x = x;
        image_config[index].y = y / 2;
        image_config[index].enable = enable;

        // округление размеров изображения до ближайшей кратности
        if((image_config[index].width*image_config[index].height) % 3)
                image_config[index].width -= image_config[index].width % 6;
        // Округление координат
        image_config[index].x = x & 0xFFFFFFFE;
        image_config[index].y = (y / 2) & 0xFFFFFFFE;

        reconfigure_image(index);
}

void PbxMtvSystem::reconfigure_image(int index)
{
        video_format_t * current_format = get_video_format(get_sdi_format(index));

        int width_in = current_format->width;
        int height_in = current_format->height;
        int width = image_config[index].width;
        int height = image_config[index].height;
        int x = image_config[index].x;
        int y = image_config[index].y;
        int enable = image_config[index].enable;

        framebuffer_start(index, 0);
        if(current_format->interlaced)
                cvi_configure(index, 0);
        else
                cvi_configure(index, 1);
        framebuffer_reconfigure(index, width, height);
        mosaic_reconfigure(index, x, y, width, height, enable);
        scaler_reconfigure(index, width_in, height_in, width, height);
        framebuffer_start(index, enable);
}

void PbxMtvSystem::reconfigure()
{
        mosaic_start(0, !dei);
        dei_configure(dei);
        cvo_reconfigure(!dei, 0);
        cvo_reconfigure(!dei, 1);
        mosaic_start(1, !dei);
}

int PbxMtvSystem::read_sdi_format(int index)
{
        uint32_t reg;

        reg = reg_read(REG_SDI_ADAPTER, index);
        if((reg&0x0f) == 0x0f)
                reg = 0x0f;
        return reg;
}

void PbxMtvSystem::sdi_format_timeout()
{
    int state_change = 0;
        for(int i=0; i<16; i++){
                int new_format = read_sdi_format(i);
                int changed = new_format!=sdi_format[i];
                sdi_format[i] = new_format;
                if(changed){
                        reconfigure_image(i);
                        state_change = 1;
                }
        }
        bars_mute();

        if(state_change) sdi_format_notify_timer.start(500);
}

void PbxMtvSystem::sdi_format_notify_timeout()
{
        emit signal_new_format();
}

int PbxMtvSystem::get_sdi_format(int index)
{
        return sdi_format[index];
}

QString PbxMtvSystem::get_sdi_format_str(int index)
{
        int format = get_sdi_format(index);

        switch(format){
        case FORMAT_SD|1:
                return "625i50";
        case FORMAT_HD|4:
                return "1080i59.94";
        case FORMAT_HD|5:
                return "1080i50";
        case FORMAT_HD|7:
                return "720p59.94";
        case FORMAT_HD|8:
                return "720p50";
        case FORMAT_3G|12:
                return "1080p59.94";
        case FORMAT_3G|13:
                return "1080p50";
        case FORMAT_HD|13:
                return "1080p25";
        default:
        case 15:
                return "LOSS";
        }
        
        return "PASS";
}

int PbxMtvSystem::get_sdi_status(int index)
{
        int format = get_sdi_format(index);

        switch(format){
        case FORMAT_SD|1:
        case FORMAT_HD|4:
        case FORMAT_HD|5:
        case FORMAT_HD|7:
        case FORMAT_HD|8:
        case FORMAT_3G|12:
        case FORMAT_HD|13:
        case FORMAT_3G|13:
                return 1;
        default:
        case 15:
                return 0;
        }
}

int PbxMtvSystem::get_sdi_hd(int index)
{
        if(get_sdi_format(index)==(FORMAT_SD|1))
                return 0;
        else
                return 1;
}

video_format_t * PbxMtvSystem::get_video_format(int id)
{
        for(int i=0; ; i++){
                if(video_format[i].id==id)
                        return &video_format[i];
                if(video_format[i].id==-1)
                        return &video_format[0];
        }
}

void PbxMtvSystem::cvi_configure(int index, int down3g)
{
        int base;
        switch(index){
        default:
        case 0:
                base = REG_CVI_0;
                break;
        case 1:
                base = REG_CVI_1;
                break;
        case 2:
                base = REG_CVI_2;
                break;
        case 3:
                base = REG_CVI_3;
                break;
        case 4:
                base = REG_CVI_4;
                break;
        case 5:
                base = REG_CVI_5;
                break;
        case 6:
                base = REG_CVI_6;
                break;
        case 7:
                base = REG_CVI_7;
                break;
        case 8:
                base = REG_CVI_8;
                break;
        case 9:
                base = REG_CVI_9;
                break;
        case 10:
                base = REG_CVI_10;
                break;
        case 11:
                base = REG_CVI_11;
                break;
        case 12:
                base = REG_CVI_12;
                break;
        case 13:
                base = REG_CVI_13;
                break;
        case 14:
                base = REG_CVI_14;
                break;
        case 15:
                base = REG_CVI_15;
                break;
        }

        reg_write(base, 0, 0
                |(down3g<<0)
        );
}

void PbxMtvSystem::dei_configure(int enable)
{
        int value;

        if(enable)
                value = 0;
        else
                value = 1;
        reg_write(REG_DEI, 0, 0
                |((1919)<<0)
                |((value)<<11)
        );
}

void PbxMtvSystem::mosaic_start(int enable, int inrelaced)
{
        reg_write(REG_MOSAIC, 0, 0
                |((inrelaced)<<0)
                |((enable)<<1)
        );
}

void PbxMtvSystem::cvo_reconfigure(int interlaced, int hdmi_sdi)
{
        int block;
        cvo_settings_t * std;

        if(hdmi_sdi==0)
                block = REG_HDMI_OUT;
        else
                block = REG_SDI_CVO;
        if(interlaced==1)
                std = &cvo_1080i50;
        else
                std = &cvo_1080p25;
        
        reg_write(block, 0, std->h_front_porch);
        reg_write(block, 1, std->h_sync);
        reg_write(block, 2, std->h_back_porch);
        reg_write(block, 3, std->total_line);
        reg_write(block, 4, (std->sdmux<<1)|std->interlaced);
        reg_write(block, 5, std->v_active);
        reg_write(block, 6, std->v_front_porch);
        reg_write(block, 7, std->v_sync);
        reg_write(block, 8, std->v_2_front_porch);
        reg_write(block, 9, std->v_2_sync);
        reg_write(block, 10, std->v_2_vsync_pixel);
        reg_write(block, 11, std->f_rising);
        reg_write(block, 12, std->f_falling);
        reg_write(block, 13, std->total_lines);
        reg_write(block, 14, std->total_lines_f2);
        reg_write(block, 15, 1);
}

void PbxMtvSystem::bars_mute()
{
        uint32_t reg = 0;

        for(int i=0; i<8; i++){
                if(!get_sdi_status(i))
                        reg |= (3<<(i*2));
        }
        reg_write(REG_BARS, 16, reg);
}

void PbxMtvSystem::set_audio_source(int index)
{
        reg_write(REG_AUDIO_SELECTOR, 0, index);
}

int PbxMtvSystem::level_value_to_db(int value)
{
        int i = 0;
        int ret = -100;

        while(level_to_db_table[i*2]!=-1){
                if(level_to_db_table[i*2] >= value){
                        ret = level_to_db_table[i*2+1];
                        break;
                }
                i++;
        }
        return ret;
}

QList<int> PbxMtvSystem::get_audio_level()
{
        QList<int> ret;

        for(int q=0; q<64; q++){ // ign 8*4=32 chnge to 16*4=64
                int i = q;
                uint32_t value = reg_read(REG_BARS, i);
                if(get_sdi_status(q/4))
                        ret.append(level_value_to_db(value));
                else
                        ret.append(-100);
        }
        return ret;
}

int PbxMtvSystem::get_motion(int index)
{
        int base;
        switch(index){
        default:
        case 0:
                base = REG_MOTION_0;
                break;
        case 1:
                base = REG_MOTION_1;
                break;
        case 2:
                base = REG_MOTION_2;
                break;
        case 3:
                base = REG_MOTION_3;
                break;
        case 4:
                base = REG_MOTION_4;
                break;
        case 5:
                base = REG_MOTION_5;
                break;
        case 6:
                base = REG_MOTION_6;
                break;
        case 7:
                base = REG_MOTION_7;
                break;
        case 8:
                base = REG_MOTION_8;
                break;
        case 9:
                base = REG_MOTION_9;
                break;
        case 10:
                base = REG_MOTION_10;
                break;
        case 11:
                base = REG_MOTION_11;
                break;
        case 12:
                base = REG_MOTION_12;
                break;
        case 13:
                base = REG_MOTION_13;
                break;
        case 14:
                base = REG_MOTION_14;
                break;
        case 15:
                base = REG_MOTION_15;
                break;
        }

        uint32_t reg = reg_read(base, 0);
        uint32_t motion_high = (reg >> 16)&0xFFFF;
        uint32_t motion_low = (reg >> 0)&0xFFFF;

        int ret = 0;
        if(motion_high > motion_low + MOTION_THR)
                ret = 1;

        return ret;
}


void PbxMtvSystem::set_dei(int enable)
{
        if(enable!=dei)
                reconfigure_timer.start(100);
        dei = enable;
}

void PbxMtvSystem::reconfigure_timeout()
{
        reconfigure();
}

void PbxMtvSystem::system_set_time(time_t time)
{
        const struct timeval tv = {time, 0};
        settimeofday(&tv, 0);
        system("/etc/init.d/hwclock.sh restart");
}