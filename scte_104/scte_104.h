#ifndef SCTE_104_H
#define SCTE_104_H

#include <QObject>
#include <QDebug>
#include <QByteArray>
#include <QLoggingCategory>
#include <QTimer>
#include <QTime>
#include <QElapsedTimer>
#include <QColor>

typedef struct{
    quint16 opID;
    QByteArray data;
} ops_message_t;

class Scte_104 : public QObject
{
    Q_OBJECT
public:
    explicit Scte_104(int channel = 0, QObject *parent = nullptr);

    enum splice_insert_type_t{
        RESERVED,
        SPLICE_START_NORMAL,
        SPLICE_START_IMMEDIATLE,
        SPLICE_END_NORMAL,
        SPLICE_END_IMMEDIATE,
        SPLICE_CANCEL
    };

    typedef struct{
        quint16 Reserved;
        quint16 messageSize;
        quint8  protocol_version;
        quint8  AS_index;
        quint8  message_number;
        quint16 DPI_PID_index;
        quint8  SCTE35_protocol_version;
        quint8  timestamp;
        quint8  num_ops;
    } multiple_operation_message_t;

    typedef struct{
        quint8  splice_insert_type;
        quint32 splice_event_id;
        quint16 unique_program_id;
        quint16 pre_roll_time;
        quint16 break_duration;
        quint8  avail_num;
        quint8  avails_expected;
        quint8  auto_return_flag;
    } splice_t;

    void parse(QByteArray message);

public slots:
    void slot_scte_104_message(QByteArray message);
    void slot_timer_in();
    void slot_timer_out();

signals:
    void signal_update_scte_104(int channel, QString text_in, QString text_out);
    void signal_scte_104_in(int channel);
    void signal_scte_104_out(int channel);

private:
    QString text_in;
    quint16 get_quint16(QByteArray message, int &pnt);
    quint8  get_quint8(QByteArray message, int &pnt);
    quint32 get_quint32(QByteArray message, int &pnt);
    void    get_timestamp(QByteArray message, int &pnt);
    void    splice_request_data(QByteArray message);

    ops_message_t get_ops_message(QByteArray message, int &pnt);

    splice_t splice;
    QTimer timer_in;
    QTimer timer_out;
    int channel;
    bool scte_Mode;    // Флаг активации расширенных логов из аргументов командной строки
};

#endif // SCTE_104_H
