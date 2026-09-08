#ifndef VGPI_API_H
#define VGPI_API_H

#include <QByteArray>
#include <QObject>


#define VGPI_PORT 10010
#define HEADER_SIZE 8
#define HANDSHAKE_SIZE 8
#define VGPI_TYPE_OFFSET 5

#define VGPI_TYPE_GET 0
#define VGPI_TYPE_GET_ANSWER 1
#define VGPI_TYPE_SET 2
#define VGPI_TYPE_NOTIFY 3

#define VGPI_FIELD_UINT16 1
#define VGPI_FIELD_INT32 2
#define VGPI_FIELD_STRING 3
#define VGPI_FIELD_SEQUENCE 4

#define VGPI_LEN_UINT16     3
#define VGPI_LEN_INT32      5
#define VGPI_LEN_GET        3
#define VGPI_LEN_GET_ANSWER 3

#define VGPI_FIELD_OFFSET_FIXED 1
#define VGPI_FIELD_OFFSET_VAR 3
#define VGPI_TYPE_SPECIFIC_OFFSET 6

#define VGPI_SEQUENCE_HDR VGPI_FIELD_OFFSET_VAR


class Vgpi_api: public QObject
{
    Q_OBJECT
public:
    explicit Vgpi_api(QObject *parent = 0);

    QByteArray vgpi_frame_uint16(int param, int data);
    QByteArray vgpi_frame_get(int param, int data);
    QByteArray vgpi_frame_get_answer(int param, int data);
    void vgpi_set_packet_size(QByteArray &arr, int len);
    void vgpi_set_type_specific(QByteArray &arr, int value);
    void vgpi_set_packet_uint16(QByteArray &arr, int value);
    void vgpi_set_packet_type(QByteArray &arr, int type);
    void parse_frame(QByteArray arr);
    int  vgpi_get_packet_type(QByteArray arr);
    int  vgpi_get_type_specific(QByteArray arr);
    QByteArray vgpi_get_data(QByteArray arr, int &type);
    int  vgpi_get_packet_size(QByteArray arr);
    int  vgpi_to_int16(QByteArray arr);
    int  vgpi_to_int32(QByteArray arr);

signals:
    void signal_parse_frame(int, int, int);

};

#endif // VGPI_API_H
