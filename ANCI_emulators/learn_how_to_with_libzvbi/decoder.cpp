#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <libzvbi.h>

#define PORT 5555

vbi_decoder *dec = nullptr;

void save_page(int pgno) {
    vbi_page page;
    if (vbi_fetch_vt_page(dec, &page, pgno, VBI_ANY_SUBNO, VBI_WST_LEVEL_1, 1, TRUE)) {
        char filename[128];
        std::sprintf(filename, "/home/root/page_%03x.bin", pgno);
        FILE *f = std::fopen(filename, "wb");
        if (f) {
            for (int row = 0; row < 24; ++row) {
                char row_buf[128];
                vbi_print_page_region(&page, row_buf, sizeof(row_buf), "UTF-8", 0, 0, row, 0, 40, 1);
                std::fprintf(f, "%s\n", row_buf);
            }
            std::fclose(f);
        }

        char png_filename[128];
        std::sprintf(png_filename, "/home/root/page_%03x.png", pgno);
        vbi_export *export_engine = vbi_export_new("png", nullptr);
        if (export_engine) {
            vbi_export_option_set(export_engine, "background", "0x000000"); 
            vbi_export_option_set(export_engine, "trim", "no"); // Не обрезать по высоте!
            if (vbi_export_file(export_engine, png_filename, &page)) {
                std::cout << "[Декодер] УСПЕХ! Создан PNG : " << png_filename << std::endl;
            }
            vbi_export_delete(export_engine);
        }
        vbi_unref_page(&page);
    } else {
        std::cerr << "[Декодер] Ошибка: либа всё ещё считает страницу 770 неполной." << std::endl;
    }
}

// Возвращаем событийный триггер либы!
static void EventHandler(vbi_event *ev, void *user_data) {
    (void)user_data;
    if (ev->type == VBI_EVENT_TTX_PAGE) {
        // Как только либа зафиксировала смену страниц на 771 — 
        // мы ОДНОВРЕМЕННО выдергиваем из кэша готовую страницу 770!
        std::cout << "[EventHandler] Либа закрыла страницу кадра!" << std::endl;
        save_page(0x770); 
    }
}

int main() {
    std::cout << "[Декодер] Инициализация libzvbi..." << std::endl;
    dec = vbi_decoder_new();
    if (!dec) return 1;

    // Регистрация коллбэка обязательна для форсирования кэша
    vbi_event_handler_register(dec, VBI_EVENT_TTX_PAGE, EventHandler, nullptr);

    std::cout << "[Декодер] Настройка сетевого UDP ресивера..." << std::endl;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return 1;

    struct sockaddr_in myaddr;
    std::memset(&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(PORT);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) return 1;

    std::vector<uint8_t> buffer(512);
    vbi_sliced p_sliced[5]; 

    std::cout << "[Декодер] Ожидание потока данных OP-47..." << std::endl;

    while (true) {
        ssize_t bytes_received = recvfrom(sock, buffer.data(), buffer.size(), 0, nullptr, nullptr);
        if (bytes_received != 242) continue;

         // === ОДНОКРАТНЫЙ СЫРОЙ ДАМП КАДРА ДЛЯ АНАЛИЗА СДВИГОВ ===
        static bool dump_printed = false;
        if (!dump_printed) {
            std::cout << "\n=== [DEBUG] СЫРОЙ UDP ПАКЕТ ОТ ИНЖЕКТОРА (" << bytes_received << " байт) ===" << std::endl;
            for (ssize_t idx = 0; idx < bytes_received; ++idx) {
                std::printf("0x%02X ", buffer[idx]);
                if ((idx + 1) % 15 == 0) std::cout << std::endl; // Разделяем строки по 15 байт
            }
            std::cout << "\n==================================================\n" << std::endl;
            dump_printed = true; // Печатаем только один раз
        }

        size_t base_pnt = 13;
        int active_lines = 0;

        for (int i = 0; i < 5; ++i) {
            size_t line_start = base_pnt + (i * 45);
            if (buffer[line_start] == 0x00 && buffer[line_start + 3] == 0x00) continue;

            // Сброс проверки строк в 0 для прохождения внутренних фильтров SDK
            p_sliced[active_lines].id = VBI_SLICED_TELETEXT_B;
            p_sliced[active_lines].line = 0; 
            std::memcpy(p_sliced[active_lines].data, &buffer[line_start + 3], 42);
            active_lines++;
        }

        if (active_lines > 0) {
            vbi_decode(dec, p_sliced, active_lines, NULL);
        }
    }

    vbi_decoder_delete(dec); close(sock); return 0;
}
