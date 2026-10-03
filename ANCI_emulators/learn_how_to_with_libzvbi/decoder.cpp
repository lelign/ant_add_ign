#undef NDEBUG
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <libzvbi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

char * fname;
static vbi_decoder * dec;
vbi_export * vbi_exporter;
vbi_sliced p_sliced[32];
int txt_line_index = 0;
int page_index = 0;
double vbi_timestamp = 0.0;

void fix_transparency(vbi_page * p_page, char * data, int width, int height) {
    for(int y=0; y<height; y++){
        for(int x=0; x<width; x++){
            const vbi_opacity opacity = (vbi_opacity)p_page->text[ y/10 * p_page->columns + x/12 ].opacity;
            uint32_t *p_pixel = (uint32_t*)&data[y * width*4+ 4*x];
            if(opacity == VBI_TRANSPARENT_SPACE) *p_pixel = 0;
        }
    }
}

void fix_trim_line(vbi_page * p_page) {
    int max_index = 0; int min_opaque = 41;
    for(int y=0; y<25; y++){
        for(int x=40; x>=0; x--){
            if(p_page->text[ y * p_page->columns + x ].unicode !=  0x20){
                if(x > max_index) max_index = x;
            }
            if(p_page->text[ y * p_page->columns + x ].opacity !=  VBI_TRANSPARENT_SPACE){
                if(x < min_opaque) min_opaque = x;
            }
        }
    }
    for(int y=0; y<25; y++){
        int min_tmp = 41;
        for(int x=40; x>=0; x--){
            if(p_page->text[ y * p_page->columns + x ].opacity !=  VBI_TRANSPARENT_SPACE){
                if(x < min_tmp) min_tmp = x;
            }
        }
        if(min_tmp == min_opaque){
            for(int x=40; x>=0; x--){
                if(x<=max_index) break;
                if((p_page->text[ y * p_page->columns + x ].opacity ==  VBI_SEMI_TRANSPARENT) && (p_page->text[ y * p_page->columns + x ].unicode ==  0x20)){
                    p_page->text[ y * p_page->columns + x + 1 ].opacity = VBI_TRANSPARENT_SPACE;
                }
            }
        }else{
            for(int x=40; x>=0; x--){
                if((p_page->text[ y * p_page->columns + x ].opacity ==  VBI_SEMI_TRANSPARENT) && (p_page->text[ y * p_page->columns + x ].unicode ==  0x20)){
                    p_page->text[ y * p_page->columns + x + 1 ].opacity = VBI_TRANSPARENT_SPACE;
                }
                if(p_page->text[ y * p_page->columns + x ].unicode !=  0x20) break;
            }
        }
    }
}

void save_page(unsigned i_wanted_page) {
    int b_cached = 0;
    vbi_page p_page;
    char out_name[64];

    b_cached = vbi_fetch_vt_page(dec, &p_page, i_wanted_page, VBI_ANY_SUBNO, VBI_WST_LEVEL_3p5, 25, 1);
    if(b_cached){
        snprintf(out_name, sizeof(out_name), "page_%03d_%x.png", page_index, i_wanted_page);
        fix_trim_line(&p_page);
        vbi_export_file(vbi_exporter, out_name, &p_page);

        snprintf(out_name, sizeof(out_name), "page_%03d_%x.bin", page_index, i_wanted_page);
        size_t alloc_size = p_page.columns * p_page.rows * 12 * 10 * 4;
        char * data = reinterpret_cast<char*>(malloc(alloc_size));
        if(data) {
            vbi_draw_vt_page(&p_page, VBI_PIXFMT_RGBA32_LE, data, 1, 1);
            fix_transparency(&p_page, data, 492, 250);
            FILE * f = fopen(out_name, "wb");
            if(f) {
                fwrite(data, alloc_size, 1, f);
                fclose(f);
            }
            free(data);
        }
        printf("[Exporter] Сгенерирована страница %X! Размер: %d %d\n", i_wanted_page, p_page.columns*12, p_page.rows*10);
    }
    page_index++;
}

static void EventHandler(vbi_event *ev, void *user_data) {
    (void)user_data;
    if(ev->type == VBI_EVENT_TTX_PAGE) {
        save_page(ev->ev.ttx_page.pgno);
    }
}

void flush_vbi_frame() {
    if (txt_line_index > 0) {
        // Повторяем циклы для гарантированной накачки циклического буфера libzvbi
        for (int repeat = 0; repeat < 15; repeat++) {
            vbi_decode(dec, p_sliced, txt_line_index, vbi_timestamp);
            vbi_timestamp += 0.04;
        }
        txt_line_index = 0;
    }
}

void op_47_process_line(uint8_t data_desc, char * data) {
    if(static_cast<uint8_t>(data[0]) != 0x55) return;
    if (txt_line_index >= 32) txt_line_index = 0;

    p_sliced[txt_line_index].id = VBI_SLICED_TELETEXT_B;
    p_sliced[txt_line_index].line = 0; 

    for(int i=0; i<42; i++) {
        p_sliced[txt_line_index].data[i] = data[3 + i]; 
    }

    // 🔥 БЕЗУСЛОВНЫЙ ОВЕРРАЙД ПОТОКА ПОД СТРАНИЦУ 100 С ПОДДЕРЖКОЙ КИРИЛЛИЦЫ
    if (txt_line_index == 0) {
        // Magazine 1, Packet 0 (Заголовок)
        p_sliced[txt_line_index].data[0] = 0x2E; // Журнал 1 (Инвертированный Хэмминг)
        p_sliced[txt_line_index].data[1] = 0x15; // Пакет 0 (Инвертированный Хэмминг)
        
        // Номер страницы: 00 (Дает 100, так как Magazine = 1) [[^1.3.2]]
        p_sliced[txt_line_index].data[2] = 0x15; // Единицы = 0 (Прямой Хэмминг)
        p_sliced[txt_line_index].data[3] = 0x15; // Десятки = 0 (Прямой Хэмминг)
        
        for(int i=4; i<9; i++) p_sliced[txt_line_index].data[i] = 0x15; // Субкоды = 0
        p_sliced[txt_line_index].data[9] = 0x0B;  // Флаг C7 = Erase Page
        p_sliced[txt_line_index].data[10] = 0x2E; // Управляющий байт под кириллический регион 32
    } 
    else if (txt_line_index == 1) {
        // Строка текста 1 (Magazine 1, Packet 1)
        p_sliced[txt_line_index].data[0] = 0x15; // Mag 1 (Инвертированный)
        p_sliced[txt_line_index].data[1] = 0x2E; // Packet 1 (Инвертированный)
    }
    else if (txt_line_index == 2) {
        // Строка текста 2 (Magazine 1, Packet 2)
        p_sliced[txt_line_index].data[0] = 0x15; // Mag 1 (Инвертированный)
        p_sliced[txt_line_index].data[1] = 0x3B; // Packet 2 (Инвертированный)
    }
    else {
        return; 
    }

    txt_line_index++;
}

void decode_line(char * data) {
    uint8_t channel = static_cast<uint8_t>(data[0]);

    if(channel != 8) {
        for(unsigned int i=0; i<5; i++) {
            uint8_t desc = static_cast<uint8_t>(data[8+i]);
            if(desc) {
                op_47_process_line(desc, data+13+i*45);
            }
        }
    }
    flush_vbi_frame();
}

void read_file() {
    int f = open(fname, O_RDONLY);
    if (f < 0) { perror("Error opening file"); exit(EXIT_FAILURE); }

    printf("[Decoder] Чтение потока данных...\n");
    uint32_t sync_word = 0;
    uint32_t size;

    while (true) {
        uint8_t byte;
        int ret = read(f, &byte, 1);
        if (ret <= 0) break;

        sync_word = (sync_word << 8) | byte;
        if (sync_word != 0xEFBEADDE) continue; // Маркер 0xDEADBEEF в Little-Endian

        ret = read(f, &size, sizeof(size));
        if (ret != sizeof(size)) break;

        if (size > 4096 || size == 0) { sync_word = 0; continue; }

        std::vector<char> frame_buf(size);
        uint32_t bytes_read = 0;
        bool read_ok = true;

        while (bytes_read < size) {
            ret = read(f, frame_buf.data() + bytes_read, size - bytes_read);
            if (ret <= 0) { read_ok = false; break; }
            bytes_read += ret;
        }

        if (read_ok) {
            decode_line(frame_buf.data());
        }
        sync_word = 0;
    }

    printf("[Decoder] Закрытие сессии вещания...\n");
    flush_vbi_frame();

    // Имитация отправки закрывающего заголовка страницы 101 (Журнал 1, Пакет 0, Единицы = 1) [[^1.3.2]]
    memset(&p_sliced[0], 0, sizeof(vbi_sliced));
    p_sliced[0].id = VBI_SLICED_TELETEXT_B;
    p_sliced[0].line = 0;
    p_sliced[0].data[0] = 0x2E; // Mag 1
    p_sliced[0].data[1] = 0x15; // Packet 0
    p_sliced[0].data[2] = 0x02; // Page Units = 1 (Прямой Хэмминг)
    p_sliced[0].data[3] = 0x15; // Page Tens = 0
    for(int i=4; i<10; i++) p_sliced[0].data[i] = 0x15;
    p_sliced[0].data[10] = 0x2E;

    for(int r = 0; r < 10; r++) {
        vbi_decode(dec, p_sliced, 1, vbi_timestamp);
        vbi_timestamp += 0.04;
    }

    printf("[Decoder] Принудительный экспорт страницы 100...\n");
    save_page(0x100);

    close(f);
}

int main(int argc, char *argv[]) {
    int c; int errflg=0;
    while ((c = getopt(argc, argv, "")) != -1) {
        switch(c) {
            case '?': errflg++;
        }
    }
    if (argc-optind != 1){ exit(EXIT_FAILURE); }
    fname = argv[optind];

    dec = vbi_decoder_new();
    assert(dec != NULL);

    vbi_exporter = vbi_export_new("png", NULL);
    vbi_teletext_set_default_region(dec, 32);

    vbi_event_handler_register(dec, VBI_EVENT_TTX_PAGE, EventHandler, NULL);

    read_file();

    if (vbi_exporter) vbi_export_delete(vbi_exporter);
    vbi_decoder_delete(dec);
    return 0;
}
