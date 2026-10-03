/*вроде это тот самый исходникб т.к. именно его я отпраавлял перед написанием OP_47_emulator*/
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <fcntl.h>

class Op47Injector {
public:
    explicit Op47Injector(const std::string& device_path) : device_path_(device_path) {}

    // Функция для расчета нечетного паритета (для обычных символов текста)
    static uint8_t calculate_odd_parity(uint8_t byte) {
        uint8_t count = 0;
        for (int i = 0; i < 7; ++i) {
            if ((byte >> i) & 1) count++;
        }
        if ((count & 1) == 0) return byte | 0x80;
        return byte & 0x7F;
    }

    // Полноценная функция кодирования Хэмминга 8/4 для служебных байт телетекста
    static uint8_t hamming_8_4_encode(uint8_t data_4bit) {
        uint8_t d1 = (data_4bit >> 0) & 1;
        uint8_t d2 = (data_4bit >> 1) & 1;
        uint8_t d3 = (data_4bit >> 2) & 1;
        uint8_t d4 = (data_4bit >> 3) & 1;

        uint8_t p1 = d1 ^ d2 ^ d4;
        uint8_t p2 = d1 ^ d3 ^ d4;
        uint8_t p3 = d2 ^ d3 ^ d4;
        uint8_t p4 = d1 ^ d2 ^ d3 ^ d4 ^ p1 ^ p2 ^ p3 ^ 1; // Защита от всех нулей

        return (p4 << 7) | (d4 << 6) | (p3 << 5) | (d3 << 4) | 
               (p2 << 3) | (d2 << 2) | (p1 << 1) | d1;
    }

    // Функция зеркалирования байта (MSB <-> LSB)
    static uint8_t reverse_bits(uint8_t b) {
        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
        return b;
    }

    // Вспомогательный метод заполнения полезной нагрузки OP-47 (45 байт)

    void format_wst_line(char* op47_payload, uint8_t magazine, uint8_t packet, const std::string& text_line, bool is_header = false) {
        op47_payload[0] = 0x55; // Синхробайт OP-47
        op47_payload[1] = 0x00;
        op47_payload[2] = 0x00;

        char* ttx = op47_payload + 3; // Начало WST-B пакета (42 байта)
        
        // 1. Framing Code (Строго 0x27, без инверсий!)
        ttx[0] = 0x27;

        // 2. Вычисляем Хэмминг 8/4 для Журнала и Пакета
        uint8_t mag_bit = (magazine & 7); 
        uint8_t mp1 = (mag_bit & 1) | ((packet & 7) << 1);
        uint8_t mp2 = ((mag_bit >> 1) & 3) | (((packet >> 3) & 3) << 2);
        
        ttx[1] = hamming_8_4_encode(mp1); // Для M1 P0 это будет строго 0x15
        ttx[2] = hamming_8_4_encode(mp2); // Для M1 P0 это будет строго 0x15

        if (is_header && packet == 0) {
            // Структура Заголовка Страницы 100
            ttx[3] = hamming_8_4_encode(0x0); // Page Units (0)
            ttx[4] = hamming_8_4_encode(0x0); // Page Tens (0)
            ttx[5] = hamming_8_4_encode(0x0); // Sub-code S1
            ttx[6] = hamming_8_4_encode(0x0); // Sub-code S2
            ttx[7] = hamming_8_4_encode(0x0); // Sub-code S3
            ttx[8] = hamming_8_4_encode(0x0); // Sub-code S4
            ttx[9] = hamming_8_4_encode(0x0); // Control bits
            ttx[10] = hamming_8_4_encode(0x4); // C7 = Erase Page

            std::string padded_text = text_line;
            padded_text.resize(31, ' ');
            for (size_t i = 0; i < 31; ++i) {
                ttx[11 + i] = calculate_odd_parity(static_cast<uint8_t>(padded_text[i]));
            }
        } else {
            // Обычная текстовая строка
            std::string padded_text = text_line;
            padded_text.resize(39, ' ');
            for (size_t i = 0; i < 39; ++i) {
                ttx[3 + i] = calculate_odd_parity(static_cast<uint8_t>(padded_text[i]));
            }
        }
    }


    bool run_test(int num_frames = 40) {
        // Открываем файл с флагом O_TRUNC (принудительное обнуление размера до 0)
        int fd = open(device_path_.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            std::cerr << "[Error] Не удалось открыть файл устройства: " << device_path_ << std::endl;
            return false;
        }

        std::cout << "[Info] Формирование идеального кадра WST-B в " << device_path_ << "..." << std::endl;

        // Передаем 3 пакета: Заголовок(0), Строка1(1), Строка2(2)
        // Общий размер структуры: 13 байт заголовков + (45 * 3 пакета) = 148 байт
        const uint32_t packet_size = 13 + (45 * 3);
        std::vector<char> frame_buf(packet_size, 0);

        // Прямая запись в заголовок структуры
        frame_buf[0] = 1;    // channel != 8
        frame_buf[1] = 0x43; // did
        frame_buf[2] = 0x02; // sdid

        // Выставим line_offset = 10, а битом 7 будем имитировать смену кадров
        uint8_t line_offset = 10;
        const uint32_t sync_marker = 0xDEADBEEF;

        for (int frame = 0; frame < num_frames; ++frame) {
            // Чередуем бит 7, чтобы в декодере срабатывал check_next_frame() 
            // и отправлял накопленный массив в vbi_decode
            uint8_t desc_bit7 = (frame % 2 == 0) ? 0 : 1;
            uint8_t data_desc = line_offset | (desc_bit7 << 7);

            // Все 3 пакета в этом кадре должны иметь ОДИНАКОВЫЙ desc,
            // тогда они накопятся в один массив p_sliced!
            frame_buf[8] = data_desc; // для пакета 0
            frame_buf[9] = data_desc; // для пакета 1
            frame_buf[10] = data_desc; // для пакета 2

            // Заполняем пакеты строго по смещениям
            format_wst_line(&frame_buf[13 + 0*45], 1, 0, "OP-47 STANDARDIZED HEADER", true);
            format_wst_line(&frame_buf[13 + 1*45], 1, 1, "HELLO! WST SYSTEM-B IS WORKING NOW!  ");
            format_wst_line(&frame_buf[13 + 2*45], 1, 2, "FRAMEWORK COMPILATION COMPLETED.     ");

            // Запись в файл
            write(fd, &sync_marker, sizeof(sync_marker));
            write(fd, &packet_size, sizeof(packet_size));
            write(fd, frame_buf.data(), packet_size);
            
            std::cout << "." << std::flush;
            usleep(40000);
        }

        std::cout << "\n[Success] Чистый дамп сгенерирован!" << std::endl;
        close(fd);
        return true;
    }

private:
    std::string device_path_;
};

int main() {
    const std::string target_device = "/dev/tsin1";
    Op47Injector injector(target_device);
    
    // Передаем 60 кадров для стабильного накопления кэша страниц в libzvbi
    if (!injector.run_test(60)) {
        return 1;
    }
    return 0;
}