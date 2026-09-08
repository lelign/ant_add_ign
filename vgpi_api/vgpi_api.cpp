#include "vgpi_api.h"

Vgpi_api::Vgpi_api(QObject *parent) :
    QObject(parent)
{
    QByteArray q;
    q = vgpi_frame_uint16(0x0304, 0x0203);
q.clear();
}


void Vgpi_api::vgpi_set_type_specific(QByteArray &arr, int value)
/**
*  Записать поле type_specifi
**/
{
    if(arr.size() < (VGPI_TYPE_SPECIFIC_OFFSET + 2)) return;

    arr[VGPI_TYPE_SPECIFIC_OFFSET    ] = (value >> 8) & 0xff;
    arr[VGPI_TYPE_SPECIFIC_OFFSET + 1] =  value & 0xff;
}


int Vgpi_api::vgpi_get_packet_size(QByteArray arr)
{
int res;

    if(arr.size() < 4) return 0;

    res   = 0;
    res  |= arr[0];
    res <<= 8;
    res  |= arr[1];
    res  <<= 8;
    res  |= arr[2];
    res <<= 8;
    res  |= arr[3];

    return res;
}

void Vgpi_api::vgpi_set_packet_size(QByteArray &arr, int len)
/**
*  Записать размер пакета
**/
{
    if(arr.size() < 4) return;

    arr.data()[3] = char(len & 0xFF);
    len >>= 8;
    arr.data()[2] = len & 0xFF;
    len >>= 8;
    arr.data()[1] = len & 0xFF;
    len >>= 8;
    arr.data()[0] = len & 0xFF;
}


void Vgpi_api::vgpi_set_packet_type(QByteArray &arr, int type)
/**
*  Записать тип пакета
**/
{
    if(arr.size() < (VGPI_TYPE_OFFSET + 1)) return;
    arr[VGPI_TYPE_OFFSET] = type;
}


void Vgpi_api::vgpi_set_packet_uint16(QByteArray &arr, int value)
/**
*  Записать поле uint16
**/
{
    if(arr.size() < (HEADER_SIZE + 3)) return;
    arr[HEADER_SIZE] = VGPI_FIELD_UINT16;
    arr[HEADER_SIZE + 1] = (value >> 8) & 0xff;
    arr[HEADER_SIZE + 2] =  value & 0xff;
}


int Vgpi_api::vgpi_get_packet_type(QByteArray arr)
/**
*  Считать тип пакета
**/
{
    if(arr.size() < (VGPI_TYPE_OFFSET + 1)) return 0;
    return arr[VGPI_TYPE_OFFSET];
}


int  Vgpi_api::vgpi_get_type_specific(QByteArray arr)
/**
*  Считать поле type_specifi
**/
{
unsigned int ret = 0;

    if(arr.size() < (VGPI_TYPE_SPECIFIC_OFFSET + 2)) return 0;
    ret |= arr[VGPI_TYPE_SPECIFIC_OFFSET];
    ret <<= 8;
    ret |= arr[VGPI_TYPE_SPECIFIC_OFFSET + 1];

    return ret;
}


int Vgpi_api::vgpi_to_int16(QByteArray arr)
{
int data;

    data = arr[0];
    data <<= 8;
    data |= arr[1];

    return data;
}


int Vgpi_api::vgpi_to_int32(QByteArray arr)
{
int data;

    data = arr[0];
    data <<= 8;
    data |= arr[1];
    data <<= 8;
    data |= arr[2];
    data <<= 8;
    data |= arr[3];

    return data;
}


QByteArray Vgpi_api::vgpi_get_data(QByteArray arr, int &type)
/**
*  Считать поле data
**/
{
int type_data;

    arr.remove(0, HEADER_SIZE);

    type_data = arr[0];
    arr.remove(0, 1);

    switch(type_data){
        case VGPI_FIELD_UINT16:
            arr = arr.left(2);
            break;
        case VGPI_FIELD_INT32:
            arr = arr.left(4);
            break;
        default:
            break;
    }

    type = type_data;

    return arr;
}

void Vgpi_api::parse_frame(QByteArray arr)
{
int type, type_specific, type_data;
    while (arr.size()) {
        type = vgpi_get_packet_type(arr);
        type_specific = vgpi_get_type_specific(arr);

        QByteArray Data = vgpi_get_data(arr, type_data);
        arr.remove(0, vgpi_get_packet_size(arr));

        switch (type_data) {
        case VGPI_FIELD_UINT16:
            emit signal_parse_frame(type, type_specific, vgpi_to_int16(Data));
            break;
        case VGPI_FIELD_INT32:
            emit signal_parse_frame(type, type_specific, vgpi_to_int32(Data));
            break;
        default:
            break;
        }
    }
}


QByteArray Vgpi_api::vgpi_frame_get_answer(int param, int data)
{
QByteArray arrBlock;
int  packet_len;

    packet_len = HEADER_SIZE + VGPI_LEN_GET_ANSWER;
    arrBlock.resize(packet_len);
    arrBlock.fill(char(0));

    vgpi_set_packet_size(arrBlock, packet_len);
    vgpi_set_packet_type(arrBlock, VGPI_TYPE_GET_ANSWER);
    vgpi_set_type_specific(arrBlock, param);
    vgpi_set_packet_uint16(arrBlock, data);

    return arrBlock;
}


QByteArray Vgpi_api::vgpi_frame_get(int param, int data)
{
QByteArray arrBlock;
int  packet_len;

    packet_len = HEADER_SIZE + VGPI_LEN_GET;
    arrBlock.resize(packet_len);
    arrBlock.fill(char(0));

    vgpi_set_packet_size(arrBlock, packet_len);
    vgpi_set_packet_type(arrBlock, VGPI_TYPE_GET);
    vgpi_set_type_specific(arrBlock, param);
    vgpi_set_packet_uint16(arrBlock, data);

    return arrBlock;
}


QByteArray Vgpi_api::vgpi_frame_uint16( int param, int data )
{
QByteArray arrBlock;
int  packet_len;

    packet_len = HEADER_SIZE + VGPI_LEN_UINT16;
    arrBlock.resize(packet_len);
    arrBlock.fill(char(0));

    vgpi_set_packet_size(arrBlock, packet_len);
    vgpi_set_packet_type(arrBlock, VGPI_TYPE_SET);
    vgpi_set_type_specific(arrBlock, param);
    vgpi_set_packet_uint16(arrBlock, data);

    return arrBlock;
}
