#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <QDebug>
#include <cstring>

// Your perfectly corrected function
void setPixelRGB(unsigned char* fb_ptr, int stride, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha = 0xFF) {
    // RGB to YUV BT.601 conversion formulas
    unsigned char y_val  = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
    unsigned char cb_val = (unsigned char)(-0.169 * r - 0.331 * g + 0.500 * b + 128);
    unsigned char cr_val = (unsigned char)(0.500 * r - 0.419 * g - 0.081 * b + 128);

    // Calculate exact pixel starting byte address
    unsigned char* pixel_ptr = fb_ptr + (y * stride) + (x * 3);

    // Check X parity for proper Chroma byte routing
    if (x % 2 == 0) {
        pixel_ptr[0] = cb_val; // Even pixel: Cb
    } else {
        pixel_ptr[0] = cr_val; // Odd pixel: Cr
    }

    pixel_ptr[1] = y_val;     // Luma (Y) is always in Byte 1
    pixel_ptr[2] = alpha;     // Transparency (Alpha) is always in Byte 2
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

    qDebug() << "2. Drawing true RGB lines via setPixelRGB...";

    // Draw RED line (X=201, unaligned coordinate check!)
    for (int y = 200; y < 500; ++y) {
        for (int x = 201; x < 211; ++x) {
            setPixelRGB(fb_ptr, stride, x, y, 255, 0, 0); // Pure Red
        }
    }

    // Draw GREEN line (X=400)
    for (int y = 200; y < 500; ++y) {
        for (int x = 400; x < 410; ++x) {
            setPixelRGB(fb_ptr, stride, x, y, 0, 255, 0); // Pure Green
        }
    }

    // Draw BLUE line (X=601, unaligned coordinate check!)
    for (int y = 200; y < 500; ++y) {
        for (int x = 601; x < 611; ++x) {
            setPixelRGB(fb_ptr, stride, x, y, 0, 0, 255); // Pure Blue
        }
    }

    munmap(fb_ptr, buffer_size);
    close(fd);
    qDebug() << "True RGB image successfully generated!";
}

int main(int argc, char *argv[]) {
    // Run our new color renderer
    drawRealRgbColors();
    return 0;
}
