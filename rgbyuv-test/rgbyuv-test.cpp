#include <QApplication>
#include <QDebug>
#include <QObject>
#include <QImage>
#include <QElapsedTimer>
#include <arm_neon.h>
#include <stdlib.h>


QT_USE_NAMESPACE

const int video_size = 1920*1080*3;

uint8_t * buffer;

int limit_color(int value)
{
        if(value > 255)
                value = 255;
        if(value < 0)
                value = 0;
        return value;
}

QRgb rgb_to_ycrcb(QRgb value)
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
                y_value = 16+(65*r_value + 129*g_value + 25*b_value)/256;
                cr_value = 128+(-37*r_value - 74*g_value + 112*b_value)/256;
                cb_value = 128+(112*r_value - 93*g_value - 18*b_value)/256;

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

void print_uint16 (uint16x8_t data) {
    int i;
    static uint16_t p[8];

    vst1q_u16 (p, data);

    for (i = 0; i < 8; i++) {
	printf ("%02d ", p[i]);
    }
    printf ("\n");
}

void print_int16 (int16x8_t data) {
    int i;
    static int16_t p[8];

    vst1q_s16 (p, data);

    for (i = 0; i < 8; i++) {
	printf ("%02d ", p[i]);
    }
    printf ("\n");
}

void print_int8 (int8x8_t data) {
    int i;
    static int8_t p[8];

    vst1_s8 (p, data);

    for (i = 0; i < 8; i++) {
	printf ("%02d ", p[i]);
    }
    printf ("\n");
}

void print_uint8 (uint8x8_t data) {
    int i;
    static uint8_t p[8];

    vst1_u8 (p, data);

    for (i = 0; i < 8; i++) {
	printf ("%02d ", p[i]);
    }
    printf ("\n");
}

const uint8_t uint8_data[] = { 
        0x44, 0x33, 0x22, 0x11,
        0x44, 0x33, 0x22, 0x11,
        0x44, 0x33, 0x22, 0x11,
        0x44, 0x33, 0x22, 0x11,
        0x44, 0x33, 0x22, 0x11,
        0x44, 0x33, 0x22, 0x11,
        0x44, 0x33, 0x22, 0x11,
        0x44, 0x33, 0x22, 0x11,
};

uint8_t uint8_out_data[] = { 
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 
};


const int16_t uint8_crcb_r_data[] = { 
        -19, 56, -19, 56, -19, 56, -19, 56, 
};

const int16_t uint8_crcb_g_data[] = { 
        -37, -46, -37, -46, -37, -46, -37, -46, 
};

const int16_t uint8_crcb_b_data[] = { 
        56, -9, 56, -9, 56, -9, 56, -9,
};

/*
        65      129     25      +16
        -37     -74     112     +128
        112     -93     -18     +128
*/
inline uint32_t test_neon()
{
        QRgb value = rgb_to_ycrcb(0x11223344);

        int32_t a_value = (value>>24)&0xFF;
        int32_t r_value = (value>>16)&0xFF;
        int32_t g_value = (value>>8)&0xFF;
        int32_t b_value = (value>>0)&0xFF;
        printf("%d %d %d %d\n", a_value, r_value, g_value, b_value);

        uint8x8x3_t ycrcb_data;

        // вычисление y
        uint8x8x4_t rgb_data = vld4_u8(uint8_data);
        ycrcb_data.val[0] = rgb_data.val[3];
        int16x8_t data_y = vmulq_s16(vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[2], vmov_n_u8(0))), vmovq_n_s16(33));
        data_y = vmlaq_s16(data_y, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[1], vmov_n_u8(0))), vmovq_n_s16(65));
        data_y = vmlaq_s16(data_y, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[0], vmov_n_u8(0))), vmovq_n_s16(13));
        data_y = vaddq_s16(data_y, vmovq_n_s16(16*128));
        ycrcb_data.val[1] = vreinterpret_u8_s8(vshrn_n_s16(data_y, 7));

        // вычисление cr
        int16x8_t data_cr = vmulq_s16(vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[2], vmov_n_u8(0))), vld1q_s16(uint8_crcb_r_data));
        data_cr = vmlaq_s16(data_cr, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[1], vmov_n_u8(0))), vld1q_s16(uint8_crcb_g_data));
        data_cr = vmlaq_s16(data_cr, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[0], vmov_n_u8(0))), vld1q_s16(uint8_crcb_b_data));
        ycrcb_data.val[2] = vreinterpret_u8_s8(vadd_s8(vshrn_n_s16(data_cr, 7), vmov_n_s8(128)));
        
        vst3_u8(uint8_out_data, ycrcb_data);

        printf("y:\n");
        print_uint8(ycrcb_data.val[1]);
        printf("cr cb:\n");
        print_uint8(ycrcb_data.val[2]);

        return 0;
}

void convert_line(QImage * img, int y, int width, uint8_t * buffer)
{
        const uint8_t * line = img->constScanLine(y);

        for(int x=0; x<width; x++){
                uint8x8x3_t ycrcb_data;
                
                // вычисление y
                uint8x8x4_t rgb_data = vld4_u8(line + x*8*4);
                ycrcb_data.val[0] = rgb_data.val[0];
                int16x8_t data_y = vmulq_s16(vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[1], vmov_n_u8(0))), vmovq_n_s16(27));
                data_y = vmlaq_s16(data_y, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[2], vmov_n_u8(0))), vmovq_n_s16(92));
                data_y = vmlaq_s16(data_y, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[3], vmov_n_u8(0))), vmovq_n_s16(9));
                ycrcb_data.val[1] = vreinterpret_u8_s8(vshrn_n_s16(data_y, 7));

                // вычисление cr
                int16x8_t data_cr = vmulq_s16(vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[1], vmov_n_u8(0))), vld1q_s16(uint8_crcb_r_data));
                data_cr = vmlaq_s16(data_cr, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[2], vmov_n_u8(0))), vld1q_s16(uint8_crcb_g_data));
                data_cr = vmlaq_s16(data_cr, vreinterpretq_s16_u16(vaddl_u8(rgb_data.val[3], vmov_n_u8(0))), vld1q_s16(uint8_crcb_b_data));
                ycrcb_data.val[2] = vreinterpret_u8_s8(vadd_s8(vshrn_n_s16(data_cr, 7), vmov_n_s8(128)));
                vst3_u8(buffer+x*3*8, ycrcb_data);
        }
}

int main(int argc, char *argv[])
{
        QImage image(1920, 1080, QImage::Format_ARGB32);
        buffer = (uint8_t*) malloc(video_size);

        QElapsedTimer timer;
        timer.start();
        for(int y=0; y<image.height(); y++)
                convert_line(&image, y, image.width()/8, buffer+y*1920*3);
        qDebug() << timer.elapsed() << "ms";

        test_neon();
        
        free(buffer);
}
