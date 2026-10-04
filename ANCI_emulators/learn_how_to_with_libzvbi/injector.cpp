#include <iostream>
#include <vector>
#include <cstring>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define OP47_DID (0x43)
#define OP47_SDID (0x02)
#define ANC_SIZE (237)
#define PORT (5555)

uint8_t encode_parity(char c) {
    uint8_t val = static_cast<uint8_t>(c) & 0x7F;
    int ones = 0;
    for (int i = 0; i < 7; ++i) {
        if ((val >> i) & 1) ones++;
    }
    if (ones % 2 == 0) val |= 0x80;
    return val;
}

int main() {
    std::cout << "[Инжектор] Настройка UDP сокета..." << std::endl;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("Ошибка сокета"); return 1; }

    struct sockaddr_in servaddr;
    std::memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    uint32_t field_counter = 0;
    std::vector<std::string> lines_text = {
        "Privet OR-47 EMULQTOR stroka 1 ", 
        "priwet ot emulqtora stroka 2   ", 
        "prowerka kirillicy stroka 3    ", 
        "tekst bez zaliwki stroka 4     ", 
        "walidaciq q-image stroka 5     "  
    };

    std::cout << "[Инжектор] Старт пакетного вещания..." << std::endl;

    while (true) {
        // Выстреливаем пачку из 30 полуполей подряд
        for (int slot_field = 0; slot_field < 30; ++slot_field) {
            field_counter++;
            std::vector<uint8_t> packet(242, 0x00); 
            
            packet[0] = 0x00; 
            packet[1] = OP47_DID;       
            packet[2] = OP47_SDID;      
            packet[3] = ANC_SIZE;       
            packet[4] = 0x08;           

            uint8_t desc_bit7 = (field_counter % 2 == 0) ? 0 : 1;
            uint8_t flag_mask = desc_bit7 << 7;
            bool is_field_2 = (field_counter % 2 == 1);
            uint16_t line_base = is_field_2 ? 316 : 10; 

            packet[8]  = static_cast<uint8_t>(line_base + 0) | flag_mask; 
            packet[9]  = static_cast<uint8_t>(line_base + 1) | flag_mask; 
            packet[10] = static_cast<uint8_t>(line_base + 2) | flag_mask; 
            packet[11] = static_cast<uint8_t>(line_base + 3) | flag_mask; 
            packet[12] = static_cast<uint8_t>(line_base + 4) | flag_mask; 

            // ЖЕСТКИЙ ТРИГГЕР: Последние 4 поля пачки переводят заголовок на 771 страницу
            bool is_flush_frame = (slot_field >= 26);
            uint8_t page_units_code = is_flush_frame ? 0x2E : 0x15; 

            size_t base_pnt = 13; 
            for (int i = 0; i < 5; ++i) {
                size_t line_start = base_pnt + (i * 45);
                packet[line_start]     = 0x55; 
                packet[line_start + 1] = 0x00; 
                packet[line_start + 2] = 0x00; 
                size_t ttx = line_start + 3;
                packet[ttx] = 0x27; 

                if (i == 0) {
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

                    std::string text = is_flush_frame ? "FLUSH TRIGGER SIGNAL 771...   " : lines_text[i]; 
                    for (size_t s = 0; s < 31; ++s) {
                        char c = (s < text.length()) ? text[s] : ' ';
                        packet[ttx + 11 + s] = encode_parity(c);
                    }
                } 
                else {
                    if (is_flush_frame) {
                        // В момент флеша полностью обнуляем строки, давая либе сигнал закрытия!
                        packet[line_start]     = 0x00;
                        packet[line_start + 1] = 0x00;
                        packet[line_start + 2] = 0x00;
                        packet[ttx]            = 0x00;
                    } else {
                        if (i == 1) { packet[ttx + 1] = 0x15; packet[ttx + 2] = 0x2E; } 
                        if (i == 2) { packet[ttx + 1] = 0x15; packet[ttx + 2] = 0x43; } 
                        if (i == 3) { packet[ttx + 1] = 0x15; packet[ttx + 2] = 0x78; } 
                        if (i == 4) { packet[ttx + 1] = 0x15; packet[ttx + 2] = 0x93; } 

                        packet[ttx + 3] = encode_parity(0x07); // Атрибут видимости Alpha White
                        std::string msg = lines_text[i];
                        for (size_t s = 0; s < 38; ++s) {
                            char c = (s < msg.length()) ? msg[s] : ' ';
                            packet[ttx + 4 + s] = encode_parity(c);
                        }
                    }
                }
            }
            for (size_t остаток = 238; остаток < 242; ++остаток) packet[остаток] = 0xA0; 

            sendto(sock, packet.data(), packet.size(), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
            usleep(20000); 
        }
        std::cout << "[Инжектор] Пачка отправлена. Сон 5 секунд..." << std::endl;
        sleep(5); 
    }
    close(sock); return 0;
}
