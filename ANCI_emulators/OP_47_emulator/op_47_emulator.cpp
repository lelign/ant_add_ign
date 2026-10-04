#include <iostream>
#include <vector>
#include <cstring>
#include <string>
#include <fcntl.h>   
#include <unistd.h>  
#include <cstdint>   
#include <cstdlib> 

#define OP47_DID (0x43)
#define OP47_SDID (0x02)

uint8_t encode_parity(char c) {
    uint8_t val = static_cast<uint8_t>(c) & 0x7F;
    int ones = 0;
    for (int i = 0; i < 7; ++i) {
        if ((val >> i) & 1) ones++;
    }
    if (ones % 2 == 0) val |= 0x80;
    return val;
}

int main(int argc, char* argv[]) {
    int target_page = 770; 

    if (argc > 1) {
        target_page = std::atoi(argv[1]);
    }

    std::cout << "[Эмулятор] Старт трансляции для страницы: " << target_page << std::endl;

    const char* DEVICE_PATH = "/dev/tsin1"; 
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        std::cerr << "Ошибка: Не удалось открыть " << DEVICE_PATH << std::endl;
        return 1;
    }

    uint8_t ANC_SIZE = 237; 

    for (int i = 0; i < 260; ++i) {
        std::vector<uint8_t> warm_packet(242, 0);
        warm_packet[0] = 0;
        warm_packet[1] = OP47_DID;
        warm_packet[2] = OP47_SDID;
        warm_packet[3] = ANC_SIZE;
        write(fd, warm_packet.data(), warm_packet.size());
        usleep(500); 
    }

    uint32_t field_counter = 0;

    // ИСПРАВЛЕНО: ФОНЕТИЧЕСКИЙ НАБОР (o->о, p->п, e->э, m->м, u->у, l->л, a->а, t->т, o->о, r->р)
    std::vector<std::string> lines_text = {
        "Privet ot OR-47 EMULQTORa stroka 1    ", // Пакет 0 (Заголовок) -> ор-47 эмулятор
        "priwet ot emulqtora stroka 2", // Пакет 1 -> привет от эмулятора
        "prowerka kirillicy stroka 3", // Пакет 2 -> проверка кириллицы
        "tekst bez zaliwki stroka 3   ", // Пакет 3 -> текст без заливки
        "walidaciq q-image "  // Пакет 4 -> валидация qimage
    };

    std::cout << "Старт финального интерлейсного вещания под страницу 770..." << std::endl;

    int target_channel = 0;
    while (true) {
        field_counter++;

        std::vector<uint8_t> packet(242, 0x00); 
        
        // === ШАГ 1: ЗАГОЛОВОК ANC ===
        int target_channel = 0;
        packet[0] = target_channel; 
        packet[1] = OP47_DID;       
        packet[2] = OP47_SDID;      
        packet[3] = ANC_SIZE; // 237       
        packet[4] = 0x08;           
        packet[5] = 0x00;           
        packet[6] = 0x00;           
        packet[7] = 0x00;           

        // === ШАГ 2: РАЗДЕЛЕНИЕ НА ПОЛЯ (С точным шагом Arria 10) ===
        uint8_t desc_bit7 = (field_counter % 2 == 0) ? 0 : 1;
        uint8_t flag_mask = desc_bit7 << 7;

        bool is_field_2 = (field_counter % 2 == 1);
        uint16_t line_base = is_field_2 ? 316 : 10; 

        packet[8]  = static_cast<uint8_t>(line_base + 0) | flag_mask; 
        packet[9]  = static_cast<uint8_t>(line_base + 1) | flag_mask; 
        packet[10] = static_cast<uint8_t>(line_base + 2) | flag_mask; 
        packet[11] = static_cast<uint8_t>(line_base + 3) | flag_mask; 
        packet[12] = static_cast<uint8_t>(line_base + 4) | flag_mask; 

        // Тайминг пульсации кадров сброса
        bool is_flush_frame = ((field_counter % 4) >= 2);
        uint8_t page_units_code = is_flush_frame ? 0x2E : 0x15; 

        // === ШАГ 3: ЗАПОЛНЕНИЕ СТРОК ТЕЛЕТЕКСТА ===
        size_t base_pnt = 13; 

        for (int i = 0; i < 5; ++i) {
            size_t line_start = base_pnt + (i * 45);

            packet[line_start]     = 0x55; 
            packet[line_start + 1] = 0x00; 
            packet[line_start + 2] = 0x00; 

            size_t ttx = line_start + 3;
            packet[ttx] = 0x27; // Framing Code

            if (i == 0) {
                // ПАКЕТ 0: Заголовок страницы
                packet[ttx + 1] = 0x15; 
                packet[ttx + 2] = 0x15; 
                packet[ttx + 3] = page_units_code; 
                packet[ttx + 4] = 0x15; 
                packet[ttx + 5] = 0x15; 
                packet[ttx + 6] = 0x15; 
                packet[ttx + 7] = 0x15; 
                packet[ttx + 8] = 0x15; 
                packet[ttx + 9] = 0x15; 
                
                // ИСПРАВЛЕНИЕ: Заменяем 0x43 (букву "Ц") на 0x1A или 0x15 с паритетом.
                // Это оставит Erase Page в памяти libzvbi, но на экране превратится в ПРОБЕЛ!
                packet[ttx + 10] = 0xA0; 

                std::string text = lines_text[i]; 
                for (size_t s = 0; s < 31; ++s) {
                    char c = (s < text.length()) ? text[s] : ' ';
                    packet[ttx + 11 + s] = encode_parity(c);
                }
            } 
            else {
                // ПАКЕТЫ 1..4: Текстовые строки субтитров
                if (is_flush_frame) {
                    packet[line_start]     = 0x00;
                    packet[line_start + 1] = 0x00;
                    packet[line_start + 2] = 0x00;
                    packet[ttx]            = 0x00;
                } else {
                    if (i == 1) { packet[ttx + 1] = 0x15; packet[ttx + 2] = 0x2E; } 
                    if (i == 2) { packet[ttx + 1] = 0x15; packet[ttx + 2] = 0x43; } 
                    if (i == 3) { packet[ttx + 1] = 0x15; packet[ttx + 2] = 0x78; } 
                    if (i == 4) { packet[ttx + 1] = 0x15; packet[ttx + 2] = 0x93; } 

                    std::string msg = lines_text[i];
                    for (size_t s = 0; s < 39; ++s) {
                        char c = (s < msg.length()) ? msg[s] : ' ';
                        packet[ttx + 3 + s] = encode_parity(c);
                    }
                }
            }
        }

        for (size_t остаток = 238; остаток < 242; ++остаток) {
            packet[остаток] = 0xA0; 
        }

        ssize_t bytes_written = write(fd, packet.data(), packet.size());
        if (bytes_written < 0) {
            std::cerr << "Ошибка записи!" << std::endl;
        }

        target_channel = (target_channel + 1) % 16;
        usleep(20000); 
    }

    close(fd);
    return 0;
}
/*
Финальная шпаргалка по заполнению строк (KOI8-R Гайд)
Поскольку теперь вы знаете, как шрифты вашей платы интерпретируют латиницу, вы можете выводить любые русские слова на строки 2, 3, 4, 5.
Вот точная фонетическая карта (Lookup Table) для вашего массива lines_text, чтобы вы могли писать любой текст:
• А → a
• Б → b
• В → w
• Г → g
• Д → d
• Е / Ё → e
• Ж → v
• З → z
• И → i
• Й → j
• К → k
• Л → l
• М → m
• Н → n
• О → o
• П → p
• Р → r
• С → s
• Т → t
• У → u
• Ф → f
• Х → h
• Ц → c
• Ч → ~
• Ш → {
• Щ → }
• Ъ → _
• Ы → y
• Ь → X
• Э → \
• Ю → @
• Я → Q
*/
