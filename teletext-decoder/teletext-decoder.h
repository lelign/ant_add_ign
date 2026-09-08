#ifndef TELTEXT_DECODER_H
#define TELTEXT_DECODER_H

#include <QtCore/QObject>
#include <QImage>
#include <libzvbi.h>

class TeletextDecoder : public QObject
{
        Q_OBJECT
public:
        TeletextDecoder();
        ~TeletextDecoder();
        void add_data(QByteArray data);
        void add_data_op42(QByteArray data);
        QImage get_page(int page);
        void page_callback(int page);
private:
        vbi_decoder * dec;
        vbi_sliced p_sliced[64];
        int txt_line_index;
        int line_offset_prev;
        int field_parity_prev;
        void op_47_process_line(uint8_t data_desc, const char * data);
        int check_next_frame(uint8_t line_offset, uint8_t field_parity);
        void fix_transparency(vbi_page * p_page, unsigned char * data, int width, int height);
        void fix_trim_line(vbi_page * p_page);
signals:
        void txtpage(int page);
};
#endif