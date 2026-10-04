#include <iostream>
#include <vector>
#include <cstring>
#include <string>
#include <fcntl.h>   
#include <unistd.h>  
#include <cstdint>   
#include <cstdlib> 
#include <sys/timerfd.h> 

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

    std::cout << "[Пакетный Эмулятор v2] Старт трансляции для страницы: " << target_page << std::endl;

    const char* DEVICE_PATH = "/dev/tsin1"; 
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        std::cerr << "Ошибка: Не удалось открыть " << DEVICE_PATH << std::endl;
        return 1;
    }

    uint8_t ANC_SIZE = 237; 

   for (int i = 0; i < 260; ++i) {
        std::vector<uint8_t> warm_packet(242, 0);
        warm_packet[0] = 0;          // Номер канала
        warm_packet[1] = OP47_DID;   // 0x43
        warm_packet[2] = OP47_SDID;  // 0x02
        warm_packet[3] = ANC_SIZE;   // 237
        write(fd, warm_packet.data(), warm_packet.size());
        usleep(500); 
    }

    uint32_t field_counter = 0;

    // ИСПРАВЛЕНИЕ: Длина первой строки скорректирована (ровно 31 символ), цифра 1 теперь внутри!
    std::vector<std::string> lines_text = {
        "Privet OR-47 EMULQTOR stroka 1 ", // Ровно 31 символ, больше не обрежется!
        "priwet ot emulqtora stroka 2   ", 
        "prowerka kirillicy stroka 3    ", 
        "tekst bez zaliwki stroka 3     ", 
        "walidaciq q-image              "  
    };

    // Настройка POSIX таймера на 5 секунд
    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd == -1) {
        std::cerr << "Ошибка создания таймера!" << std::endl;
        close(fd);
        return 1;
    }

    struct itimerspec timer_config;
    timer_config.it_value.tv_sec = 5;
    timer_config.it_value.tv_nsec = 0;
    timer_config.it_interval.tv_sec = 5;
    timer_config.it_interval.tv_nsec = 0;

    if (timerfd_settime(tfd, 0, &timer_config, nullptr) == -1) {
        std::cerr << "Ошибка запуска таймера!" << std::endl;
        close(tfd);
        close(fd);
        return 1;
    }

    std::cout << "Старт расширенного пакетного вещания (интервал: 5 секунд)..." << std::endl;

    int target_channel = 0;
    while (true) {
        
        // ИСПРАВЛЕНИЕ: Увеличиваем пачку вещания до 10 полуполей подряд.
        // 6 полуполей стабильно льют текст страницы 770, и только последние 4 поля делают сброс.
        // Это даст софтверному FIFO время полностью наполнить кэш всеми 5 строками субтитров!
        // ИСПРАВЛЕНИЕ: Увеличиваем пачку до 30 полуполей подряд (около 600 мс непрерывного потока)
        // Это даст FIFO буферу приложения достаточно времени, чтобы накопить все текстовые строки!
        for (int slot_field = 0; slot_field < 30; ++slot_field) {
            field_counter++;

            std::vector<uint8_t> packet(242, 0x00); 
            
            // === ШАГ 1: ЗАГОЛОВОК ANC ===
            packet[0] = target_channel; 
            packet[1] = OP47_DID;       
            packet[2] = OP47_SDID;      
            packet[3] = ANC_SIZE;       
            packet[4] = 0x08;           
            packet[5] = 0x00;           
            packet[6] = 0x00;           
            packet[7] = 0x00;           

            // === ШАГ 2: РАЗДЕЛЕНИЕ НА ПОЛЯ ===
            uint8_t desc_bit7 = (field_counter % 2 == 0) ? 0 : 1;
            uint8_t flag_mask = desc_bit7 << 7;

            bool is_field_2 = (field_counter % 2 == 1);
            uint16_t line_base = is_field_2 ? 316 : 10; 

            packet[8]  = static_cast<uint8_t>(line_base + 0) | flag_mask; 
            packet[9]  = static_cast<uint8_t>(line_base + 1) | flag_mask; 
            packet[10] = static_cast<uint8_t>(line_base + 2) | flag_mask; 
            packet[11] = static_cast<uint8_t>(line_base + 3) | flag_mask; 
            packet[12] = static_cast<uint8_t>(line_base + 4) | flag_mask; 

            // Из 30 полей: первые 26 полей стабильно льют текст, а последние 4 поля делают сброс
            bool is_flush_frame = (slot_field >= 26);
            uint8_t page_units_code = is_flush_frame ? 0x2E : 0x15; 

            // === ШАГ 3: ЗАПОЛНЕНИЕ СТРОК ТЕЛЕТЕКСТА ===
            size_t base_pnt = 13; 

            for (int i = 0; i < 5; ++i) {
                size_t line_start = base_pnt + (i * 45);

                packet[line_start]     = 0x55; 
                packet[line_start + 1] = 0x00; 
                packet[line_start + 2] = 0x00; 

                size_t ttx = line_start + 3;
                packet[ttx] = 0x27; 

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
                    packet[ttx + 10] = 0xA0; 

                    std::string text = lines_text[i]; 
                    for (size_t s = 0; s < 31; ++s) {
                        char c = (s < text.length()) ? text[s] : ' ';
                        packet[ttx + 11 + s] = encode_parity(c);
                    }
                } 
                else {
                    // ПАКЕТЫ 1..4: Текстовые строки субтитров (ЖЕСТКИЕ СТАБИЛЬНЫЕ БАЙТЫ)
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

            write(fd, packet.data(), packet.size());
            usleep(20000); 
        }


        std::cout << "Пакет страницы 770 успешно отправлен для входа " << target_channel << std::endl;
        target_channel = (target_channel + 1) % 16;

        // Блокирующее ожидание таймера на 5 секунд
        uint64_t timer_missed_ticks = 0;
        read(tfd, &timer_missed_ticks, sizeof(timer_missed_ticks));
    }

    close(tfd);
    close(fd);
    return 0;
}






// work, scanned, triggered but one line (first) only
// #include <iostream>
// #include <vector>
// #include <cstring>
// #include <string>
// #include <fcntl.h>   
// #include <unistd.h>  
// #include <cstdint>   
// #include <cstdlib> 
// #include <sys/timerfd.h> 

// #define OP47_DID (0x43)
// #define OP47_SDID (0x02)

// uint8_t encode_parity(char c) {
//     uint8_t val = static_cast<uint8_t>(c) & 0x7F;
//     int ones = 0;
//     for (int i = 0; i < 7; ++i) {
//         if ((val >> i) & 1) ones++;
//     }
//     if (ones % 2 == 0) val |= 0x80;
//     return val;
// }

// int main(int argc, char* argv[]) {
//     int target_page = 770; 

//     if (argc > 1) {
//         target_page = std::atoi(argv[1]);
//     }

//     std::cout << "[Пакетный Эмулятор] Старт трансляции для страницы: " << target_page << std::endl;

//     const char* DEVICE_PATH = "/dev/tsin1"; 
//     int fd = open(DEVICE_PATH, O_RDWR);
//     if (fd == -1) {
//         std::cerr << "Ошибка: Не удалось открыть " << DEVICE_PATH << std::endl;
//         return 1;
//     }

//     uint8_t ANC_SIZE = 237; 

//     for (int i = 0; i < 260; ++i) {
//         std::vector<uint8_t> warm_packet(242, 0);
//         warm_packet[0] = 0;
//         warm_packet[1] = OP47_DID;
//         warm_packet[2] = OP47_SDID;
//         warm_packet[3] = ANC_SIZE;
//         write(fd, warm_packet.data(), warm_packet.size());
//         usleep(500); 
//     }

//     uint32_t field_counter = 0;

//     std::vector<std::string> lines_text = {
//         "Privet ot OR-47 EMULQTORa stroka 1    ", 
//         "priwet ot emulqtora stroka 2", 
//         "prowerka kirillicy stroka 3", 
//         "tekst bez zaliwki stroka 3   ", 
//         "walidaciq q-image "  
//     };

//     // Настройка POSIX таймера на 5 секунд
//     int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
//     if (tfd == -1) {
//         std::cerr << "Ошибка создания таймера!" << std::endl;
//         close(fd);
//         return 1;
//     }

//     struct itimerspec timer_config;
//     timer_config.it_value.tv_sec = 5;
//     timer_config.it_value.tv_nsec = 0;
//     timer_config.it_interval.tv_sec = 5;
//     timer_config.it_interval.tv_nsec = 0;

//     if (timerfd_settime(tfd, 0, &timer_config, nullptr) == -1) {
//         std::cerr << "Ошибка запуска таймера!" << std::endl;
//         close(tfd);
//         close(fd);
//         return 1;
//     }

//     std::cout << "Старт интерлейсного пакетного вещания (каждые 5 секунд)..." << std::endl;

//     int target_channel = 0;
//     while (true) {
        
//         // === ИСПРАВЛЕНИЕ: ОТПРАВЛЯЕМ МИНИ-ПАЧКУ ИЗ 4 ПОЛУПОЛЕЙ ПОДРЯД ===
//         // Это восстановит тайминги интерлейса для libzvbi и вызовет коллбэк!
//         for (int slot_field = 0; slot_field < 4; ++slot_field) {
//             field_counter++;

//             std::vector<uint8_t> packet(242, 0x00); 
            
//             // === ШАГ 1: ЗАГОЛОВОК ANC ===
//             packet[0] = target_channel; 
//             packet[1] = OP47_DID;       
//             packet[2] = OP47_SDID;      
//             packet[3] = ANC_SIZE; // 237       
//             packet[4] = 0x08;           
//             packet[5] = 0x00;           
//             packet[6] = 0x00;           
//             packet[7] = 0x00;           

//             // === ШАГ 2: РАЗДЕЛЕНИЕ НА ПОЛЯ (20мс задержка между ними внутри пачки) ===
//             uint8_t desc_bit7 = (field_counter % 2 == 0) ? 0 : 1;
//             uint8_t flag_mask = desc_bit7 << 7;

//             bool is_field_2 = (field_counter % 2 == 1);
//             uint16_t line_base = is_field_2 ? 316 : 10; 

//             packet[8]  = static_cast<uint8_t>(line_base + 0) | flag_mask; 
//             packet[9]  = static_cast<uint8_t>(line_base + 1) | flag_mask; 
//             packet[10] = static_cast<uint8_t>(line_base + 2) | flag_mask; 
//             packet[11] = static_cast<uint8_t>(line_base + 3) | flag_mask; 
//             packet[12] = static_cast<uint8_t>(line_base + 4) | flag_mask; 

//             // Пульсация кадров сброса строго внутри этой локальной пачки
//             bool is_flush_frame = (slot_field >= 2);
//             uint8_t page_units_code = is_flush_frame ? 0x2E : 0x15; 

//             // === ШАГ 3: ЗАПОЛНЕНИЕ СТРОК ТЕЛЕТЕКСТА ===
//             size_t base_pnt = 13; 

//             for (int i = 0; i < 5; ++i) {
//                 size_t line_start = base_pnt + (i * 45);

//                 packet[line_start]     = 0x55; 
//                 packet[line_start + 1] = 0x00; 
//                 packet[line_start + 2] = 0x00; 

//                 size_t ttx = line_start + 3;
//                 packet[ttx] = 0x27; // Framing Code

//                 if (i == 0) {
//                     packet[ttx + 1] = 0x15; 
//                     packet[ttx + 2] = 0x15; 
//                     packet[ttx + 3] = page_units_code; 
//                     packet[ttx + 4] = 0x15; 
//                     packet[ttx + 5] = 0x15; 
//                     packet[ttx + 6] = 0x15; 
//                     packet[ttx + 7] = 0x15; 
//                     packet[ttx + 8] = 0x15; 
//                     packet[ttx + 9] = 0x15; 
//                     packet[ttx + 10] = 0xA0; 

//                     std::string text = lines_text[i]; 
//                     for (size_t s = 0; s < 31; ++s) {
//                         char c = (s < text.length()) ? text[s] : ' ';
//                         packet[ttx + 11 + s] = encode_parity(c);
//                     }
//                 } 
//                 else {
//                     if (is_flush_frame) {
//                         packet[line_start]     = 0x00;
//                         packet[line_start + 1] = 0x00;
//                         packet[line_start + 2] = 0x00;
//                         packet[ttx]            = 0x00;
//                     } else {
//                         if (i == 1) { packet[ttx + 1] = 0x15; packet[ttx + 2] = 0x2E; } 
//                         if (i == 2) { packet[ttx + 1] = 0x15; packet[ttx + 2] = 0x43; } 
//                         if (i == 3) { packet[ttx + 1] = 0x15; packet[ttx + 2] = 0x78; } 
//                         if (i == 4) { packet[ttx + 1] = 0x15; packet[ttx + 2] = 0x93; } 

//                         std::string msg = lines_text[i];
//                         for (size_t s = 0; s < 39; ++s) {
//                             char c = (s < msg.length()) ? msg[s] : ' ';
//                             packet[ttx + 3 + s] = encode_parity(c);
//                         }
//                     }
//                 }
//             }

//             for (size_t остаток = 238; остаток < 242; ++остаток) {
//                 packet[остаток] = 0xA0; 
//             }

//             write(fd, packet.data(), packet.size());
            
//             // Внутри пачки держим честные 20мс вещания для синхронизации полей libzvbi
//             usleep(20000); 
//         }

//         std::cout << "Страница 770 отправлена для входа " << target_channel << std::endl;
//         target_channel = (target_channel + 1) % 16;

//         // === БЛОКИРУЮЩЕЕ ОЖИДАНИЕ ТАЙМЕРА НА 5 СЕКУНД ===
//         uint64_t timer_missed_ticks = 0;
//         read(tfd, &timer_missed_ticks, sizeof(timer_missed_ticks));
//     }

//     close(tfd);
//     close(fd);
//     return 0;
// }


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
