#include "scte_104.h"
#include <QCoreApplication>

static QLoggingCategory category("Scte_104");

Scte_104::Scte_104(int channel, QObject *parent) : QObject(parent)
{
    scte_Mode = QCoreApplication::arguments().contains("--scte");
    if(scte_Mode){
        qCDebug(category).noquote() << QString("Instance for channel %1. Creating...").arg(channel);
    }
    

    this->channel = channel;

    timer_in.setSingleShot(true);
    connect(&timer_in, &QTimer::timeout, this, &Scte_104::slot_timer_in);

    timer_out.setSingleShot(true);
    connect(&timer_out, &QTimer::timeout, this, &Scte_104::slot_timer_out);

}

void Scte_104::slot_scte_104_message(QByteArray message)
{
//    qDebug() << "scte_104_message. Channel:" << channel << "-" << message.toHex(' ');
    parse(message);
}

void Scte_104::parse(QByteArray message)
{
multiple_operation_message_t multiple_operation_message;

    int pnt = 0;
    int DID = get_quint8(message, pnt);     Q_UNUSED(DID)
    int SDID= get_quint8(message, pnt);     Q_UNUSED(SDID)
    int tmp = get_quint8(message, pnt);     Q_UNUSED(tmp)
    int Payload = get_quint8(message, pnt); Q_UNUSED(Payload)

    multiple_operation_message.Reserved                 = get_quint16(message, pnt);
    multiple_operation_message.messageSize              = get_quint16(message, pnt);
    multiple_operation_message.protocol_version         = get_quint8 (message, pnt);
    multiple_operation_message.AS_index                 = get_quint8 (message, pnt);
    multiple_operation_message.message_number           = get_quint8 (message, pnt);
    multiple_operation_message.DPI_PID_index            = get_quint16(message, pnt);
    multiple_operation_message.SCTE35_protocol_version  = get_quint8 (message, pnt);
    get_timestamp(message, pnt);
    multiple_operation_message.num_ops                  = get_quint8 (message, pnt);

    for(int i = 0; i < multiple_operation_message.num_ops; i++){
        ops_message_t ops_message = get_ops_message(message, pnt);
        if(ops_message.opID == 0x0101) splice_request_data(ops_message.data);
    }

}

void Scte_104::splice_request_data(QByteArray message)
{
    int pnt = 0;

    splice.splice_insert_type = get_quint8 (message, pnt);
    splice.splice_event_id    = get_quint32(message, pnt);
    splice.unique_program_id  = get_quint16(message, pnt);
    splice.pre_roll_time      = get_quint16(message, pnt);
    splice.break_duration     = get_quint16(message, pnt);
    splice.avail_num          = get_quint8 (message, pnt);
    splice.avails_expected    = get_quint8 (message, pnt);
    splice.auto_return_flag   = get_quint8 (message, pnt);

    switch(splice.splice_insert_type){
        case RESERVED :
            break;
        case SPLICE_START_NORMAL :
            timer_in.start(splice.pre_roll_time);
            break;
        case SPLICE_START_IMMEDIATLE :
            break;
        case SPLICE_END_NORMAL :
            timer_out.start(splice.pre_roll_time);
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
