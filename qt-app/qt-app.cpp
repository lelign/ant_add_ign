#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <QDebug>
#include <cstring>

// Функция попиксельной отрисовки с корректной синхронизацией YUV 4:2:2 + Alpha
void setPixelRGB(unsigned char* fb_ptr, int stride, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha = 0xFF) {
    // 1. Вычисляем координаты пары пикселей
    int even_x = (x / 2) * 2;
    int odd_x  = even_x + 1;

    // 2. Считаем YUV для текущего запрашиваемого пикселя (BT.601)
    unsigned char y_val  = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
    unsigned char cb_val = (unsigned char)(-0.169 * r - 0.331 * g + 0.500 * b + 128);
    unsigned char cr_val = (unsigned char)(0.500 * r - 0.419 * g - 0.081 * b + 128);

    // 3. Находим адрес конкретно этого пикселя для записи Яркости и Альфы
    unsigned char* pixel_ptr = fb_ptr + (y * stride) + (x * 3);
    pixel_ptr[1] = y_val;     // Яркость индивидуальна
    pixel_ptr[2] = alpha;     // Прозрачность индивидуальна

    // 4. СИНХРОНИЗАЦИЯ ЦВЕТНОСТИ (Убираем зеленые пиксели-артефакты)
    unsigned char* even_pixel_ptr = fb_ptr + (y * stride) + (even_x * 3);
    unsigned char* odd_pixel_ptr  = fb_ptr + (y * stride) + (odd_x * 3);

    // Записываем одну и ту же цветность в оба пикселя аппаратного макро-блока
    even_pixel_ptr[0] = cb_val; // Cb уходит на четную позицию
    odd_pixel_ptr[0]  = cr_val; // Cr уходит на нечетную позицию
}

void drawRealRgbColors() {
    int fd = open("/dev/mtv-overlay", O_RDWR);
    if (fd < 0) {
        qDebug() << "Error opening driver!";
        return;
    }

    size_t buffer_size = 1920 * 1080 * 3;
    unsigned char* fb_ptr = (unsigned char*)mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (fb_ptr == MAP_FAILED) {
        qDebug() << "Mmap failed!";
        close(fd);
        return;
    }

    int width = 1920;
    int height = 1080;
    int stride = 5760; 

    qDebug() << "1. Clearing screen to deep YUV black...";
    for (int y = 0; y < height; ++y) {
        unsigned char* row_ptr = fb_ptr + (y * stride);
        for (int x = 0; x < width; ++x) {
            int idx = x * 3;
            row_ptr[idx + 0] = 0x80; // Chroma Neutral
            row_ptr[idx + 1] = 0x00; // Luma Black
            row_ptr[idx + 2] = 0xFF; // Alpha Full Opacity
        }
    }

        qDebug() << "2. Drawing true RGB lines with PERFECT YUV 4:2:2 alignment...";

    int start_y = 200;
    int end_y = 800;
    int total_height = end_y - start_y; 

    for (int y = start_y; y < end_y; ++y) {
        float factor = 1.0f - ((float)(y - start_y) / total_height);
        unsigned char current_alpha_val = (unsigned char)(255 * factor);

        // ИСПРАВЛЕНО: X от 200 до 220 (Строго четные границы пар для Красной полосы)
        for (int x = 200; x < 220; ++x) {
            setPixelRGB(fb_ptr, stride, x, y, 255, 0, 0, current_alpha_val);
        }

        // ОСТАВЛЕНО: X от 400 до 420 (Изначально правильная четная Зеленая полоса)
        for (int x = 400; x < 420; ++x) {
            setPixelRGB(fb_ptr, stride, x, y, 0, 255, 0, current_alpha_val);
        }

        // ИСПРАВЛЕНО: X от 600 до 620 (Строго четные границы пар для Синей полосы)
        for (int x = 600; x < 620; ++x) {
            setPixelRGB(fb_ptr, stride, x, y, 0, 0, 255, current_alpha_val);
        }
    }

    munmap(fb_ptr, buffer_size);
    close(fd);
    qDebug() << "v2 True RGB image with pure Alpha gradients successfully generated!";
}

int main(int argc, char *argv[]) {
    drawRealRgbColors();
    return 0;
}
