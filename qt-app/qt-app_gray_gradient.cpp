//Запись 0x80, 0x80 во все цветовые каналы дает идеальный серый/белый цвет

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <QDebug>
#include <cstring>

// Перечисление для выбора цвета линии
enum YuvColor {
    YUV_WHITE,
    YUV_RED,
    YUV_GREEN,
    YUV_BLUE
};

// Вспомогательная функция для получения байт Y, Cr, Cb на основе enum
void getColorBytes(YuvColor color, unsigned char &y_val, unsigned char &cr_val, unsigned char &cb_val) {
    cr_val = 0x80; // Всегда нейтраль
    cb_val = 0x80; // Всегда нейтраль
    
    switch (color) {
        case YUV_RED:
            y_val = 0x40; // Темно-серая линия (25% яркости)
            break;
        case YUV_GREEN:
            y_val = 0x80; // Серая линия (50% яркости)
            break;
        case YUV_BLUE:
            y_val = 0xC0; // Светло-серая линия (75% яркости)
            break;
        case YUV_WHITE:
        default:
            y_val = 0xFF; // Чистый белый (100% яркости)
            break;
    }
}


// Функция рисования вертикальной линии с поддержкой цвета
void drawVerticalLine(unsigned char* fb_ptr, int stride, int start_x, int start_y, int length, int thickness_pixels, YuvColor color) {
    int aligned_x = (start_x / 2) * 2;
    int aligned_thickness = ((thickness_pixels + 1) / 2) * 2;

    unsigned char y_val, cr_val, cb_val;
    getColorBytes(color, y_val, cr_val, cb_val);

    for (int y = start_y; y < (start_y + length); ++y) {
        unsigned char* row_ptr = fb_ptr + (y * stride);
        
        for (int dx = 0; dx < aligned_thickness; dx += 2) {


            int idx = (aligned_x + dx) * 3;

// Первый пиксель пары (Строго Cr / Y / Alpha)
row_ptr[idx + 0] = cr_val; // Красный канал цветности живет здесь
row_ptr[idx + 1] = y_val;
row_ptr[idx + 2] = 0xFF;   // Alpha

// Второй пиксель пары (Строго Cb / Y / Alpha)
row_ptr[idx + 3] = cb_val; // Синий канал цветности живет здесь
row_ptr[idx + 4] = y_val;
row_ptr[idx + 5] = 0xFF;   // Alpha
            
            /*int idx = (aligned_x + dx) * 3;
            
            // Пиксель 1 из пары (Cr / Y / Alpha)
            row_ptr[idx + 0] = cr_val;
            row_ptr[idx + 1] = y_val;
            row_ptr[idx + 2] = 0xFF; // Alpha
            
            // Пиксель 2 из пары (Cb / Y / Alpha)
            row_ptr[idx + 3] = cb_val;
            row_ptr[idx + 4] = y_val;
            row_ptr[idx + 5] = 0xFF; // Alpha*/
        }
    }
}

// Функция рисования горизонтальной линии с поддержкой цвета
void drawHorizontalLine(unsigned char* fb_ptr, int stride, int start_x, int start_y, int length_pixels, int thickness_lines, YuvColor color) {
    int aligned_x = (start_x / 2) * 2;
    int aligned_length = ((length_pixels + 1) / 2) * 2;

    unsigned char y_val, cr_val, cb_val;
    getColorBytes(color, y_val, cr_val, cb_val);

    for (int dt = 0; dt < thickness_lines; ++dt) {
        unsigned char* row_ptr = fb_ptr + ((start_y + dt) * stride);
        
        for (int dx = 0; dx < aligned_length; dx += 2) {


            int idx = (aligned_x + dx) * 3;

// Первый пиксель пары (Строго Cr / Y / Alpha)
row_ptr[idx + 0] = cr_val; // Красный канал цветности живет здесь
row_ptr[idx + 1] = y_val;
row_ptr[idx + 2] = 0xFF;   // Alpha

// Второй пиксель пары (Строго Cb / Y / Alpha)
row_ptr[idx + 3] = cb_val; // Синий канал цветности живет здесь
row_ptr[idx + 4] = y_val;
row_ptr[idx + 5] = 0xFF;   // Alpha

            /**/
        }
    }
}

void drawYuvGeometry() {
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

    qDebug() << "Filling background with YUV Black...";
    for (int y = 0; y < height; ++y) {
        unsigned char* row_ptr = fb_ptr + (y * stride);
        for (int x = 0; x < width; x += 2) {
            int idx = x * 3;
            row_ptr[idx + 0] = 0x80; row_ptr[idx + 1] = 0x00; row_ptr[idx + 2] = 0xFF;
            row_ptr[idx + 3] = 0x80; row_ptr[idx + 4] = 0x00; row_ptr[idx + 5] = 0xFF;
        }
    }

    qDebug() << "Drawing colored test geometry...";

    // 1. КРАСНАЯ вертикальная линия (X=200, Y=100, длина=300, толщина=6)
    drawVerticalLine(fb_ptr, stride, 200, 100, 300, 6, YUV_RED);

    // 2. ЗЕЛЕНАЯ вертикальная линия (X=300, Y=100, длина=300, толщина=6)
    drawVerticalLine(fb_ptr, stride, 300, 100, 300, 6, YUV_GREEN);

    // 3. СИНЯЯ вертикальная линия (X=400, Y=100, длина=300, толщина=6)
    drawVerticalLine(fb_ptr, stride, 400, 100, 300, 6, YUV_BLUE);

    // 4. Длинная белая ГОРИЗОНТАЛЬНАЯ линия снизу для проверки (X=100, Y=500, длина=800, высота=4)
    drawHorizontalLine(fb_ptr, stride, 100, 500, 800, 4, YUV_WHITE);

    // 5. КРАСНАЯ горизонтальная линия (X=100, Y=550, длина=800, высота=4)
    drawHorizontalLine(fb_ptr, stride, 100, 550, 800, 4, YUV_RED);

    munmap(fb_ptr, buffer_size);
    close(fd);
    qDebug() << "Colored geometry successfully drawn!";
}

int main(int argc, char *argv[]) {
    drawYuvGeometry();
    return 0;
}



/*
//ok four white lines
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <QDebug>
#include <cstring>

// Функция рисования вертикальной линии произвольной толщины
void drawVerticalLine(unsigned char* fb_ptr, int stride, int start_x, int start_y, int length, int thickness_pixels) {
    // В YUV 4:2:2 мы всегда работаем парами пикселей, поэтому округляем параметры до четных значений
    int aligned_x = (start_x / 2) * 2;
    int aligned_thickness = ((thickness_pixels + 1) / 2) * 2; // Толщина кратна 2 пикселям

    for (int y = start_y; y < (start_y + length); ++y) {
        unsigned char* row_ptr = fb_ptr + (y * stride);
        
        // Рисуем линию заданной толщины по горизонтали
        for (int dx = 0; dx < aligned_thickness; dx += 2) {
            int idx = (aligned_x + dx) * 3;
            
            // Пиксель 1 из пары
            row_ptr[idx + 0] = 0x80; // Cr
            row_ptr[idx + 1] = 0xFF; // Y (White)
            row_ptr[idx + 2] = 0xFF; // Alpha
            
            // Пиксель 2 из пары
            row_ptr[idx + 3] = 0x80; // Cb
            row_ptr[idx + 4] = 0xFF; // Y (White)
            row_ptr[idx + 5] = 0xFF; // Alpha
        }
    }
}

// Функция рисования горизонтальной линии произвольной толщины (высоты)
void drawHorizontalLine(unsigned char* fb_ptr, int stride, int start_x, int start_y, int length_pixels, int thickness_lines) {
    int aligned_x = (start_x / 2) * 2;
    int aligned_length = ((length_pixels + 1) / 2) * 2;

    // Цикл по толщине (заполняем несколько строк подряд)
    for (int dt = 0; dt < thickness_lines; ++dt) {
        unsigned char* row_ptr = fb_ptr + ((start_y + dt) * stride);
        
        // Заполняем линию слева направо парами пикселей
        for (int dx = 0; dx < aligned_length; dx += 2) {
            int idx = (aligned_x + dx) * 3;
            
            row_ptr[idx + 0] = 0x80; // Cr
            row_ptr[idx + 1] = 0xFF; // Y (White)
            row_ptr[idx + 2] = 0xFF; // Alpha
            
            row_ptr[idx + 3] = 0x80; // Cb
            row_ptr[idx + 4] = 0xFF; // Y (White)
            row_ptr[idx + 5] = 0xFF; // Alpha
        }
    }
}

void drawYuvGeometry() {
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

    qDebug() << "Filling background with YUV Black...";
    for (int y = 0; y < height; ++y) {
        unsigned char* row_ptr = fb_ptr + (y * stride);
        for (int x = 0; x < width; x += 2) {
            int idx = x * 3;
            row_ptr[idx + 0] = 0x80; row_ptr[idx + 1] = 0x00; row_ptr[idx + 2] = 0xFF;
            row_ptr[idx + 3] = 0x80; row_ptr[idx + 4] = 0x00; row_ptr[idx + 5] = 0xFF;
        }
    }

    qDebug() << "Drawing test geometry...";

    // 1. Рисуем ТОЛСТУЮ вертикальную линию (X=200, Y=100, длина=300, толщина=10 пикселей)
    drawVerticalLine(fb_ptr, stride, 200, 100, 300, 10);

    // 2. Рисуем еще одну Тонкую вертикальную линию рядом (X=250, Y=100, длина=300, толщина=2 пикселя)
    drawVerticalLine(fb_ptr, stride, 250, 100, 300, 2);

    // 3. Рисуем ТОЛСТУЮ ГОРИЗОНТАЛЬНУЮ линию (X=400, Y=200, длина=500 пикселей, толщина/высота=8 строк)
    drawHorizontalLine(fb_ptr, stride, 400, 200, 500, 8);

    // 4. Рисуем тонкую горизонтальную линию (X=400, Y=230, длина=500 пикселей, толщина/высота=1 строка)
    drawHorizontalLine(fb_ptr, stride, 400, 230, 500, 1);

    munmap(fb_ptr, buffer_size);
    close(fd);
    qDebug() << "Geometry successfully drawn!";
}

int main(int argc, char *argv[]) {
    drawYuvGeometry();
    return 0;
}
*/



/*
//thin white line OK
// #include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <QDebug>
#include <cstring>

void drawYuvTestPattern() {
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

    qDebug() << "Filling background with YUV Black...";

    for (int y = 0; y < height; ++y) {
        unsigned char* row_ptr = fb_ptr + (y * stride);
        for (int x = 0; x < width; x += 2) {
            int idx = x * 3;
            row_ptr[idx + 0] = 0x80; // Cr
            row_ptr[idx + 1] = 0x00; // Y 
            row_ptr[idx + 2] = 0xFF; // Alpha
            
            row_ptr[idx + 3] = 0x80; // Cb
            row_ptr[idx + 4] = 0x00; // Y 
            row_ptr[idx + 5] = 0xFF; // Alpha
        }
    }

    qDebug() << "Drawing white vertical line at X=101...";

    int target_x = 100; 
    for (int y = 100; y < 200; ++y) {
        unsigned char* row_ptr = fb_ptr + (y * stride);
        int idx = target_x * 3;
        
        row_ptr[idx + 0] = 0x80; 
        row_ptr[idx + 1] = 0xFF; 
        row_ptr[idx + 2] = 0xFF; 
        
        row_ptr[idx + 3] = 0x80; 
        row_ptr[idx + 4] = 0xFF; 
        row_ptr[idx + 5] = 0xFF; 
    }

    munmap(fb_ptr, buffer_size);
    close(fd);
    qDebug() << "YUV 4:2:2 + Alpha pattern successfully drawn!";
}

// ТОЧКА ВХОДА, КОТОРУЮ ИСКАЛ ЛИНКЕР Поки SDK
int main(int argc, char *argv[]) {
    // Вызываем тестовую отрисовку YUV
    drawYuvTestPattern();
    return 0;
}
*/


/*
//rose
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <QDebug>
#include <cstring>

void drawYuvTestPattern() {
    // 1. Открываем устройство драйвера mtv-overlay
    int fd = open("/dev/mtv-overlay", O_RDWR);
    if (fd < 0) {
        qDebug() << "Error opening driver!";
        return;
    }

    // 2. Мапим видеопамять (размер строго 1920 * 1080 * 3 = 6220800 байт)
    size_t buffer_size = 1920 * 1080 * 3;
    unsigned char* fb_ptr = (unsigned char*)mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (fb_ptr == MAP_FAILED) {
        qDebug() << "Mmap failed!";
        close(fd);
        return;
    }

    // Параметры геометрии кадра
    int width = 1920;
    int height = 1080;
    int stride = 5760; // 1920 пикселей * 3 байта

    qDebug() << "Filling background with YUV Black...";

    // 3. Заполняем весь буфер правильным YUV-черным фоном попарно
    for (int y = 0; y < height; ++y) {
        unsigned char* row_ptr = fb_ptr + (y * stride);
        
        // Шагаем по 2 пикселя (6 байт за итерацию)
        for (int x = 0; x < width; x += 2) {
            int idx = x * 3;
            
            // Первый пиксель пары: Cr / Y / Alpha
            row_ptr[idx + 0] = 0x80; // Cr (Neutral)
            row_ptr[idx + 1] = 0x00; // Y  (Black)
            row_ptr[idx + 2] = 0xFF; // Alpha (Full opacity)
            
            // Второй пиксель пары: Cb / Y / Alpha
            row_ptr[idx + 3] = 0x80; // Cb (Neutral)
            row_ptr[idx + 4] = 0x00; // Y  (Black)
            row_ptr[idx + 5] = 0xFF; // Alpha (Full opacity)
        }
    }

    qDebug() << "Drawing white vertical line at X=101...";

    // 4. Рисуем вертикальную линию на X=101 (длиной от Y=100 до Y=200)
    // Так как 101 — нечетный, мы обязаны модифицировать всю пару пикселей (100 и 101),
    // чтобы не разрушить структуру Cb/Cr. Обе точки станут белыми.
    int target_x = 100; // Округляем до четного начала пары
    
    for (int y = 100; y < 200; ++y) {
        unsigned char* row_ptr = fb_ptr + (y * stride);
        int idx = target_x * 3;
        
        // Первый пиксель пары (X=100) -> Белый
        row_ptr[idx + 0] = 0x80; // Cr
        row_ptr[idx + 1] = 0xFF; // Y (White)
        row_ptr[idx + 2] = 0xFF; // Alpha
        
        // Второй пиксель пары (X=101) -> Белый
        row_ptr[idx + 3] = 0x80; // Cb
        row_ptr[idx + 4] = 0xFF; // Y (White)
        row_ptr[idx + 5] = 0xFF; // Alpha
    }

    // 5. Завершение работы
    munmap(fb_ptr, buffer_size);
    close(fd);
    qDebug() << "YUV 4:2:2 + Alpha pattern successfully drawn!";
}
*/