#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <QDebug>
#include <cstring>

void runByteDiagnostic() {
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

    qDebug() << "1. Clearing screen to YUV-Black...";
    for (int y = 0; y < height; ++y) {
        unsigned char* row_ptr = fb_ptr + (y * stride);
        for (int x = 0; x < width; ++x) {
            int idx = x * 3;
            row_ptr[idx + 0] = 0x80; // Neutral Chroma
            row_ptr[idx + 1] = 0x00; // Black Luma (Y)
            row_ptr[idx + 2] = 0x80; // Neutral Chroma
        }
    }

    qDebug() << "2. Drawing 3 test lines for Byte 0, Byte 1, Byte 2...";

    // Линия 1 (X=200): Изменяем только БАЙТ 0
    for (int y = 100; y < 400; ++y) {
        unsigned char* row_ptr = fb_ptr + (y * stride);
        for (int x = 200; x < 210; ++x) {
            int idx = x * 3;
            row_ptr[idx + 0] = 0xFF; // Max out Byte 0
            row_ptr[idx + 1] = 0x80; 
            row_ptr[idx + 2] = 0x80; 
        }
    }

    // Линия 2 (X=400): Изменяем только БАЙТ 1
    for (int y = 100; y < 400; ++y) {
        unsigned char* row_ptr = fb_ptr + (y * stride);
        for (int x = 400; x < 410; ++x) {
            int idx = x * 3;
            row_ptr[idx + 0] = 0x80; 
            row_ptr[idx + 1] = 0xFF; // Max out Byte 1
            row_ptr[idx + 2] = 0x80; 
        }
    }

    // Линия 3 (X=600): Изменяем только БАЙТ 2
    for (int y = 100; y < 400; ++y) {
        unsigned char* row_ptr = fb_ptr + (y * stride);
        for (int x = 600; x < 610; ++x) {
            int idx = x * 3;
            row_ptr[idx + 0] = 0x80; 
            row_ptr[idx + 1] = 0x80; 
            row_ptr[idx + 2] = 0xFF; // Max out Byte 2
        }
    }

    munmap(fb_ptr, buffer_size);
    close(fd);
    qDebug() << "Diagnostic pattern successfully written!";
}

int main(int argc, char *argv[]) {
    runByteDiagnostic();
    return 0;
}
