#undef NDEBUG
#include <stdio.h>
#include <stdlib.h>
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
int line_offset_prev = 0;
int field_parity_prev = 0;

int check_next_frame(uint8_t line_offset, uint8_t field_parity)
{
        int ret = 0;

        if((field_parity==1)&&(field_parity_prev==0))
                ret = 1;
        /* ситуация с 1 полем, содержащим телетекст */
        if((field_parity==field_parity_prev)&&(line_offset<=line_offset_prev))
                ret = 1;

        line_offset_prev = line_offset;
        field_parity_prev = field_parity;

        return ret;
}

static int parse_opt(int argc, char *argv[])
{
        int c;
        int errflg=0;
        extern char *optarg;

        while ((c = getopt(argc, argv, "")) != -1) {
                switch(c) {
                case '?':
                        fprintf(stderr, "Unrecognized option: -%c\n", optopt);
                        errflg++;
                }
        }
        if (argc-optind != 1){
                fprintf(stderr, "Expected arguments after options\n");
                exit(EXIT_FAILURE);
        }
        fname = argv[optind];
        return errflg;
}

void op_47_process_line(uint8_t data_desc, char * data)
{
        uint8_t line_offset;
        uint8_t field_parity;

        if(data[0] != 0x55)
                return;

        line_offset = data_desc & 0x1F;
        field_parity = !((data_desc>>7) & 0x1);

        p_sliced[txt_line_index].id = VBI_SLICED_TELETEXT_B;
        p_sliced[txt_line_index].line = line_offset + (field_parity ? 0 : 313);
        for(int i=0; i<42; i++){
                p_sliced[txt_line_index].data[i] = data[3 + i];
        }
        txt_line_index ++;

        if(check_next_frame(line_offset, field_parity)){
                if(txt_line_index > 0)
                        vbi_decode(dec, p_sliced, txt_line_index, 0);
                txt_line_index = 0;
        }
}

static unsigned char reverse(unsigned char b) {
        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
        return b;
}

void op_42_process_line(char * data)
{
        uint8_t line_offset;
        uint8_t field_parity;

        line_offset = data[0] & 0x1F;
        field_parity = ((data[0]>>7) & 0x1);

        p_sliced[txt_line_index].id = VBI_SLICED_TELETEXT_B;
        p_sliced[txt_line_index].line = line_offset + (field_parity ? 0 : 313);

        for(int i=0; i<42; i++){
                p_sliced[txt_line_index].data[i] = reverse(data[2 + i]);
        }
        txt_line_index ++;
        if(check_next_frame(line_offset, field_parity)){
                if(txt_line_index > 0)
                        vbi_decode(dec, p_sliced, txt_line_index, 0);
                txt_line_index = 0;
        }
}

void decode_line(char * data)
{
        uint8_t channel;
        uint8_t did;
        uint16_t sdid;
        uint8_t dc;
        uint8_t id1;
        uint8_t id2;
        uint8_t len;
        uint8_t txt_format;

        channel = data[0];

        if(channel != 8){
                did = data[1];
                sdid = data[2];
                dc = data[3];
                id1 = data[4];
                id2 = data[5];
                len = data[6];
                txt_format = data[7];
                
                for(unsigned int i=0; i<5; i++){
                        uint8_t desc = data[8+i];
                        if(desc)
                                op_47_process_line(desc, data+13+i*45);
                }
        }else{
                op_42_process_line(data+1);
        }
}

void read_file()
{
        int f;
        char buf[256];
        uint32_t size;
        int ret=0;

        f = open(fname, O_RDONLY);
        assert(f>0);

        while(ret>=0){
                ret = read(f, &size, sizeof(size));
                if(ret!=sizeof(size))
                        break;
                ret = read(f, buf, size);
                if(ret!=size)
                        break;
                decode_line(buf);
        }

        close(f);
}

int page_index = 0;

void fix_transparency(vbi_page * p_page, char * data, int width, int height)
{
        for(int y=0; y<height; y++){
                for(int x=0; x<width; x++){
                        const vbi_opacity opacity = p_page->text[ y/10 * p_page->columns + x/12 ].opacity;
                        uint32_t *p_pixel = (uint32_t*)&data[y * width*4+ 4*x];
                        if(opacity == VBI_TRANSPARENT_SPACE)
                                *p_pixel = 0;
                }
        }
}

void fix_trim_line(vbi_page * p_page)
{
        int max_index = 0;
        int min_opaque = 41;
        max_index = 0;
        min_opaque = 41;
        for(int y=0; y<25; y++){
                for(int x=40; x>=0; x--){
                        if(p_page->text[ y * p_page->columns + x ].unicode !=  0x20){
                                if(x > max_index)
                                        max_index = x;
                        }
                        if(p_page->text[ y * p_page->columns + x ].opacity !=  VBI_TRANSPARENT_SPACE){
                                    if(x < min_opaque)
                                        min_opaque = x;
                        }
                }
        }
        for(int y=0; y<25; y++){
                int min_tmp = 41;
                for(int x=40; x>=0; x--){
                        if(p_page->text[ y * p_page->columns + x ].opacity !=  VBI_TRANSPARENT_SPACE){
                                if(x < min_tmp)
                                        min_tmp = x;
                        }
                }
                if(min_tmp == min_opaque){
                        for(int x=40; x>=0; x--){
                                if(x<=max_index)
                                        break;
                                if((p_page->text[ y * p_page->columns + x ].opacity ==  VBI_SEMI_TRANSPARENT)
                                &&(p_page->text[ y * p_page->columns + x ].unicode ==  0x20)){
                                        p_page->text[ y * p_page->columns + x + 1 ].opacity = VBI_TRANSPARENT_SPACE;
                                }
                        }
                }else{
                        for(int x=40; x>=0; x--){
                                if((p_page->text[ y * p_page->columns + x ].opacity ==  VBI_SEMI_TRANSPARENT)
                                &&(p_page->text[ y * p_page->columns + x ].unicode ==  0x20)){
                                        p_page->text[ y * p_page->columns + x + 1 ].opacity = VBI_TRANSPARENT_SPACE;
                                }
                                if(p_page->text[ y * p_page->columns + x ].unicode !=  0x20){
                                        break;
                                }
                        }
                }
        }
}

void save_page(unsigned i_wanted_page)
{
        int b_cached = 0;
        vbi_page p_page;
        char fname[64];

        b_cached = vbi_fetch_vt_page(dec, &p_page,
                i_wanted_page,
                VBI_ANY_SUBNO, VBI_WST_LEVEL_3p5,
                25, 1 );
        if(b_cached){
                sprintf(fname, "page_%03d_%x.png", page_index, i_wanted_page);
                fix_trim_line(&p_page);
                vbi_export_file(vbi_exporter, fname, &p_page);

                sprintf(fname, "page_%03d_%x.bin", page_index, i_wanted_page);
                char * data = malloc(p_page.columns*p_page.rows*12*10*4);
                vbi_draw_vt_page(&p_page, VBI_PIXFMT_RGBA32_LE, data, 1, 1);
                fix_transparency(&p_page, data, 492, 250);
                FILE * f;
                f = fopen(fname, "w+");
                fwrite(data, p_page.columns*p_page.rows*12*10*4, 1, f);
                fclose(f);
                free(data);
                printf("%d %d\n", p_page.columns*12, p_page.rows*10);
        }

        page_index ++;
}

static void EventHandler( vbi_event *ev, void *user_data )
{
        if( ev->type == VBI_EVENT_TTX_PAGE )
        {
                printf("Page %03x.%02x \n",
                        ev->ev.ttx_page.pgno,
                        ev->ev.ttx_page.subno & 0xFF);
                save_page(ev->ev.ttx_page.pgno);
        }
}

int main(int argc, char *argv[])
{
        if(parse_opt(argc, argv))
                return 1;
        dec = vbi_decoder_new();
        vbi_exporter = vbi_export_new ("png", NULL);
        vbi_teletext_set_default_region(dec, 32);
        vbi_event_handler_register( dec, VBI_EVENT_TTX_PAGE | VBI_EVENT_NETWORK |
                                VBI_EVENT_CAPTION | VBI_EVENT_TRIGGER |
                                VBI_EVENT_ASPECT | VBI_EVENT_PROG_INFO | VBI_EVENT_NETWORK_ID |
                                0 , EventHandler, 0 );
        read_file();
        vbi_decoder_delete(dec);
}