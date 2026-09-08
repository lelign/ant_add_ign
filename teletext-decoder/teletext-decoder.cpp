#include <QLoggingCategory>
#include "teletext-decoder.h"

static QLoggingCategory category("Teletext Decoder");

static void EventHandler( vbi_event *ev, void *user_data )
{
        TeletextDecoder * c = (TeletextDecoder *) user_data;

        if( ev->type == VBI_EVENT_TTX_PAGE )
        {
                c->page_callback(vbi_bcd2dec(ev->ev.ttx_page.pgno));
        }
}

TeletextDecoder::TeletextDecoder()
{
        dec = vbi_decoder_new();
        vbi_teletext_set_default_region(dec, 32);
        txt_line_index = 0;
        line_offset_prev = 0;
        field_parity_prev = 0;
        vbi_event_handler_register( dec, VBI_EVENT_TTX_PAGE, EventHandler, this );
}

TeletextDecoder::~TeletextDecoder()
{
        vbi_decoder_delete(dec);
}

void TeletextDecoder::page_callback(int page)
{
        emit txtpage(page);
}

int TeletextDecoder::check_next_frame(uint8_t line_offset, uint8_t field_parity)
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

void TeletextDecoder::op_47_process_line(uint8_t data_desc, const char * data)
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

void TeletextDecoder::add_data(QByteArray data)
{
        for(unsigned int i=0; i<5; i++){
                uint8_t desc = data.at(7+i);
                if(desc)
                        op_47_process_line(desc, data.constData()+12+i*45);
        }
}

static unsigned char reverse(unsigned char b) {
        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
        return b;
}

void TeletextDecoder::add_data_op42(QByteArray data)
{
        uint8_t line_offset;
        uint8_t field_parity;

        line_offset = data.at(0) & 0x1F;
        field_parity = ((data.at(0)>>7) & 0x1);

        p_sliced[txt_line_index].id = VBI_SLICED_TELETEXT_B;
        p_sliced[txt_line_index].line = line_offset + (field_parity ? 0 : 313);

        for(int i=0; i<42; i++){
                p_sliced[txt_line_index].data[i] = reverse(data.at(2 + i));
        }
        txt_line_index ++;
        if(check_next_frame(line_offset, field_parity)){
                if(txt_line_index > 0)
                        vbi_decode(dec, p_sliced, txt_line_index, 0);
                txt_line_index = 0;
        }
}

void TeletextDecoder::fix_transparency(vbi_page * p_page, unsigned char * data, int width, int height)
{
        for(int y=0; y<height; y++){
                for(int x=0; x<width; x++){
                        const vbi_opacity opacity = (vbi_opacity) p_page->text[ y/10 * p_page->columns + x/12 ].opacity;
                        uint32_t *p_pixel = (uint32_t*)&data[y * width*4+ 4*x];
                        if(opacity == VBI_TRANSPARENT_SPACE)
                                *p_pixel = 0;
                }
        }
}

void TeletextDecoder::fix_trim_line(vbi_page * p_page)
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

QImage TeletextDecoder::get_page(int page)
{
        int b_cached = 0;
        vbi_page p_page;
        b_cached = vbi_fetch_vt_page(dec, &p_page,
                vbi_dec2bcd(page),
                VBI_ANY_SUBNO, VBI_WST_LEVEL_3p5, 25, 1);
        if(!b_cached)
                return QImage();
        QImage ret(492, 250, QImage::Format_RGBA8888);
        fix_trim_line(&p_page);
        vbi_draw_vt_page(&p_page, VBI_PIXFMT_RGBA32_LE, ret.bits(), 1, 1);
        fix_transparency(&p_page, ret.bits(), 492, 250);
        return ret.convertToFormat(QImage::Format_ARGB32);
}
