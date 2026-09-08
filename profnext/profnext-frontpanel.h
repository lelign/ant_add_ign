#ifndef FRONTPANEL_H
#define FRONTPANEL_H

#include <QtCore/QObject>
#include <QDebug>
#include <QString>
#include <QList>
#include <QtSerialPort/QSerialPort>
#include <QTimer>
#include "cpu-cmd.h"

#define UART_TIMEOUT (5000)

class ProfnextFrontPanel : public QObject
{
        Q_OBJECT
public:
        void cmd_profnext(T_CMD_TYPE type, T_CMD_NUM idx, T_NODE_DYN_ATTR *attr, char *data);
        void parsing_cpu_cmd_profnext(cpu_cmd_profnext_t *cmd_profnext);
        void cmd_ack();
        // uint8_t flag_reset;
        explicit ProfnextFrontPanel(QString port);
        ~ProfnextFrontPanel();
private:
        QSerialPort *uart;
        QByteArray uart_data;
        void serial_read();

        QTimer timeout_timer;
        QList <QByteArray> msg_queue;
        int msg_in_progress;
        void queue_data(QByteArray msg_data);
        void send_msg();
        int read_5566();
        void process_msg(QByteArray *msg_data);
Q_SIGNALS:
        void indexed_string_event(uint8_t idx, uint16_t val);
private Q_SLOTS:
        void uart_timeout();
};

#endif
