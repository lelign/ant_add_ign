#include "scte_104.h"
#include <QCoreApplication>

static QLoggingCategory category("Scte_104");

Scte_104::Scte_104(int channel, QObject *parent) : QObject(parent)
{
    scte_Mode = QCoreApplication::arguments().contains("--scte");
    if(scte_Mode){
        qCDebug(category).noquote() << QString("\tInstance for channel %1. Creating...").arg(channel);
    }
    

    this->channel = channel;

    timer_in.setSingleShot(true);
    connect(&timer_in, &QTimer::timeout, this, &Scte_104::slot_timer_in);

    timer_out.setSingleShot(true);
    connect(&timer_out, &QTimer::timeout, this, &Scte_104::slot_timer_out);

}

void Scte_104::slot_scte_104_message(QByteArray message)
{
    // qDebug() << "slot_scte_104_message scte_104_message. Channel:" << channel << "-" << message.toHex(' '); // ok
    parse(message);
}

void Scte_104::parse(QByteArray message)
{
multiple_operation_message_t multiple_operation_message;
// на примере 
// DID: 0x41 
// SDID: 0x07
// 41 07 28 08 ff ff 00 27 00 00 33 00 00 00 00 02 01 01 00 0e 01 04 00 00 6e 00 00 1f 40 00 96 00 00 01 01 0a 00 05 01 00 00 00 01 5d
    int pnt = 0;
    int DID = get_quint8(message, pnt);     Q_UNUSED(DID)       // Считает 0x41 (S2010_DID)
    int SDID= get_quint8(message, pnt);     Q_UNUSED(SDID)      // Считает 0x07 (S2010_SDID)
    int tmp = get_quint8(message, pnt);     Q_UNUSED(tmp)       // Считает 0x28 (Размер)
    int Payload = get_quint8(message, pnt); Q_UNUSED(Payload)   // Считает 0x08 (Доп. заголовок)
    // 4 байта DID, SDID, tmp, Payload, успешно считываются, смещают указатель pnt на 4 позиции вперед и уходят в Q_UNUSED, 
    // так как AncReader их уже отвалидировал
    // Дальше указатель pnt встает на байты самого SCTE-104:
    multiple_operation_message.Reserved                 = get_quint16(message, pnt); // поле называется opID глобального заголовка и всегда равно 0xFFFF
    multiple_operation_message.messageSize              = get_quint16(message, pnt); // в шестнадцатеричной системе это 39 байт — ровно столько данных идет следом
    multiple_operation_message.protocol_version         = get_quint8 (message, pnt); //   00
    multiple_operation_message.AS_index                 = get_quint8 (message, pnt); //   00
    multiple_operation_message.message_number           = get_quint8 (message, pnt); //  33 (уникальный порядковый номер сообщения, в следующих примерах он инкрементируется: 34, 35 и т.д.).
    multiple_operation_message.DPI_PID_index            = get_quint16(message, pnt); //  00 00
    multiple_operation_message.SCTE35_protocol_version  = get_quint8 (message, pnt); //  00
    get_timestamp(message, pnt);                                                     // структура таймстампа (в примерах это 00 00 02, занимает 3 байта).
    multiple_operation_message.num_ops                  = get_quint8 (message, pnt); // 01 — сообщает парсеру, что внутри сообщения содержится ровно 1 операция.

    qCDebug(category).noquote() << QString("\tparse. Channel %1 message_number %2 num_ops %3")
                                                        .arg(channel)
                                                        .arg(multiple_operation_message.message_number)
                                                        .arg(multiple_operation_message.num_ops);
    for(int i = 0; i < multiple_operation_message.num_ops; i++){
        ops_message_t ops_message = get_ops_message(message, pnt); // get_ops_message парсит заголовок операции. Смещение pnt доходит до байт 01 01
        // qCDebug(category).noquote() << QString("\tparse. ops_message %1").arg(ops_message);
        if(ops_message.opID == 0x0101) {
            splice_request_data(ops_message.data); // 01 01 (это шестнадцатеричный идентификатор операции splice_request согласно стандарту SCTE-104).
            // qDebug() << "parse. Channel:" << channel << "ops_message.data" << ops_message.data;
        }
    }

}

void Scte_104::splice_request_data(QByteArray message)
{
    int pnt = 0;
    // message : 01 00 00 6e 00 00 1f 40 00 96 00 00 01 01
    splice.splice_insert_type = get_quint8 (message, pnt); // 01  константа SPLICE_START_NORMAL — стандартное начало рекламного блока.
    splice.splice_event_id    = get_quint32(message, pnt); // 00 00 6e 00 decimal 28160 (уникальный ID этого рекламного события)
    splice.unique_program_id  = get_quint16(message, pnt); // 00 00 (ID программы).
    splice.pre_roll_time      = get_quint16(message, pnt); // 1f 40. decimal 8000 8 секунд preroll.
    splice.break_duration     = get_quint16(message, pnt);
    splice.avail_num          = get_quint8 (message, pnt); // 00 96 decimal 150 (длительность блока в десятых долях секунды, то есть реклама будет идти 15 секунд)   
    splice.avails_expected    = get_quint8 (message, pnt); // оставшиеся байты 00, 00, 01
    splice.auto_return_flag   = get_quint8 (message, pnt); // оставшиеся байты 00, 00, 01

    switch(splice.splice_insert_type){
        case RESERVED :
            break;
        case SPLICE_START_NORMAL :
            qCDebug(category).noquote() << QString("\ttimer_in.start.").arg(splice.pre_roll_time);
            timer_in.start(splice.pre_roll_time); // (Вход в рекламу) timer start at 8 секунд preroll
            break;
        case SPLICE_START_IMMEDIATLE :
            break;
        case SPLICE_END_NORMAL :
            qCDebug(category).noquote() << QString("\ttimer_out.start.").arg(splice.pre_roll_time);
            timer_out.start(splice.pre_roll_time); // (Выход из рекламы) timer start at 8 секунд preroll
            break;
        case SPLICE_END_IMMEDIATE :
            break;
        case SPLICE_CANCEL:
            break;
        default:
            break;
    }
}

ops_message_t Scte_104::get_ops_message(QByteArray message, int &pnt)
{
quint16 data_length;
ops_message_t ops_message;

    ops_message.opID = get_quint16(message, pnt);

    data_length = get_quint16(message, pnt);

    for(int i = 0; i < data_length; i++){
        ops_message.data.append(message[pnt++]);
    }

    return ops_message;
}

void Scte_104::slot_timer_in()
{
    text_in = QTime::currentTime().toString("HH:mm:ss");
    emit signal_update_scte_104(channel, text_in, "");
    emit signal_scte_104_in(channel);
}

void Scte_104::slot_timer_out()
{
QString text_out;

    text_out = QTime::currentTime().toString("HH:mm:ss");
    emit signal_update_scte_104(channel, text_in, text_out);
    emit signal_scte_104_out(channel);
}

quint32 Scte_104::get_quint32(QByteArray message, int &pnt)
{
quint32 data = 0;

    data = message[pnt++] & 0xff;
    data <<=  8;
    data |= message[pnt++] & 0xff;
    data <<=  8;
    data |= message[pnt++] & 0xff;
    data <<=  8;
    data |= message[pnt++] & 0xff;

    return data;
}

quint16 Scte_104::get_quint16(QByteArray message, int &pnt)
{
quint16 data;

    data = message[pnt++] & 0xff;
    data <<=  8;
    data |= message[pnt++] & 0xff;

    return data;
}

quint8 Scte_104::get_quint8(QByteArray message, int &pnt)
{
    return message[pnt++] & 0xff;
}


void Scte_104::get_timestamp(QByteArray message, int &pnt)
{
int time_type;

    time_type = message[pnt];

    switch(time_type){
        case 0:  pnt += 1;  break;
        case 1:  pnt += 6;  break;
        case 2:  pnt += 4;  break;
        case 3:  pnt += 2;  break;
    }
}
