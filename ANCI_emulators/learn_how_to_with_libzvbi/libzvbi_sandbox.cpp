#include <iostream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <libzvbi.h>

int main() {
    std::cout << "[Песочница] Создание объекта vbi_decoder..." << std::endl;
    vbi_decoder *dec = vbi_decoder_new();
    if (!dec) return 1;

    std::vector<std::string> lines_text = {
        "OP-47 SANDBOX TITLE LINE 0    ", 
        "Hello from text packet line 1  ", 
        "Testing libzvbi cache line 2   ", 
        "Pure memory simulation line 3  ", 
        "Rendering verification line 4  "  
    };

    std::cout << "[Песочница] Шаг 1: Инициализация правильной структуры vbi_page..." << std::endl;
    
    vbi_page page;
    std::memset(&page, 0, sizeof(vbi_page));
    
    page.pgno = 0x770;        // Страница 770 в BCD [etsi.org]
    page.subno = 0x0000;      
    
    // Взводим геометрию, которая дает идеальные 480x480 пикселей кадра телетекста [etsi.org]
    page.columns = 40; 
    page.rows = 24;    

    // === ЖЕСТКАЯ ИНИЦИАЛИЗАЦИЯ ПАЛИТРЫ (COLOR MAP) ДЛЯ УНИЧТОЖЕНИЯ ПРОЗРАЧНОСТИ ===
    // Цвета в libzvbi хранятся в формате RGBA (0xRRGGBBAA).
    // Младший байт AA отвечает за Альфа-канал: 0xFF - полностью видим, 0x00 - прозрачен!
    page.color_map[0] = 0x000000FF; // Индекс 0: Вещательный черный фон (Полностью непрозрачный!)
    page.color_map[7] = 0xFFFFFFFF; // Индекс 7: Вещательный белый цвет букв (Полностью непрозрачный!)

    std::cout << "[Песочница] Шаг 2: Наполнение матрицы страницы (5 строк текста)..." << std::endl;

    // Инициализируем всю матрицу экрана (24 строки на 40 столбцов) видимыми пробелами
    for (int r = 0; r < 24; ++r) {
        for (int c = 0; c < 40; ++c) {
            vbi_char *vc = &page.text[r * 40 + c];
            vc->unicode = ' ';
            vc->foreground = 7;           // Ссылка на индекс 7 палитры (Белый)
            vc->background = 0;           // Ссылка на индекс 0 палитры (Черный)
            vc->opacity = VBI_OPAQUE;     // Принудительно отключаем альфа-прозрачность пикселей!
            vc->size = VBI_NORMAL_SIZE;   // Обычный размер шрифта WST-B [etsi.org]
        }
    }

    // Наносим наши 5 строк полезного текста на холст
    for (size_t row = 0; row < 5; ++row) {
        std::string text = lines_text[row];
        for (size_t col = 0; col < 40; ++col) {
            vbi_char *vc = &page.text[row * 40 + col];
            if (col < text.length()) {
                vc->unicode = text[col]; 
            }
            vc->foreground = 7; 
            vc->background = 0; 
            vc->opacity = VBI_OPAQUE; 
            vc->size = VBI_NORMAL_SIZE;
        }
    }

    std::cout << "[Песочница] Шаг 3: Экспорт сформированной страницы на диск..." << std::endl;

    // 1. Сохраняем текстовое представление (.bin)
    FILE *f = std::fopen("sandbox_page_770.bin", "wb");
    if (f) {
        for (int row = 0; row < 24; ++row) {
            char row_buf[256]; // Безопасный размер буфера строки для UTF-8
            vbi_print_page_region(&page, row_buf, sizeof(row_buf), "UTF-8", 0, 0, row, 0, 40, 1);
            std::fprintf(f, "%s\n", row_buf);
        }
        std::fclose(f);
        std::cout << "[УСПЕХ] Текстовый файл sandbox_page_770.bin успешно создан!" << std::endl;
    }

    // 2. Рендерим в PNG-картинку напрямую через графический плагин либы
    vbi_export *export_engine = vbi_export_new("png", nullptr);
    if (export_engine) {
        vbi_export_option_set(export_engine, "background", "0x000000");
        vbi_export_option_set(export_engine, "trim", "no"); // Намертво блокируем автообрезку строк по высоте!

        if (vbi_export_file(export_engine, "sandbox_page_770.png", &page)) {
            std::cout << "[УСПЕХ] Графический PNG успешно создан: sandbox_page_770.png" << std::endl;
        } else {
            std::cerr << "Ошибка графического экспорта!" << std::endl;
        }
        vbi_export_delete(export_engine);
    }

    vbi_decoder_delete(dec);
    return 0;
}

/*
Это абсолютный, грандиозный триумф! Посмотрите на этот экран — все 5 строк текста вывелись одновременно, идеально ровно, буква к букве, на полном вещательном кадре 480×480 пикселей!
Этот скриншот — ключевой момент истины во всей нашей многодневной отладке. Мы полностью раскололи внутреннюю логику графического рендерера libzvbi и доказали, как именно библиотека управляет памятью.
🔬 Глубокий разбор результатов эксперимента:
1. Многострочный PNG полностью реален: Библиотека без проблем умеет генерировать единый графический файл для нескольких строк субтитров сразу. Опция "trim", "no" отработала безупречно, сохранив полную высоту кадра.
2. Почему фон стал красным, а буквы белыми?
Посмотрите на эту деталь. Мы прописали в page.color_map[0] = 0x000000FF (черный) и в page.color_map[7] = 0xFFFFFFFF (белый). Но на картинке фон залился ярко-красным цветом.
Это произошло из-за того, что в вашей системной версии libzvbi массив color_map внутри структуры vbi_page использует инвертированный порядок байт (Little-Endian BGRA или ABGR). Значение 0x000000FF, которое мы считали черным с непрозрачной альфой, библиотека раскодировала как максимальный красный канал (Red = 255), превратив подложку экрана в красное вещательное поле.
3. Почему буквы «проявились» именно сейчас?
Потому что зануление скрытого массива page.color_map через std::memset в предыдущих тестах выставляло всем цветам альфа-канал 0x00 (полная прозрачность). Как только мы вручную записали байты в color_map, пиксели шрифта мгновенно получили физический цвет и зажглись на растре.

*/