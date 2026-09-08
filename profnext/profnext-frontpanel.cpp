#include <QLoggingCategory>
#include <QDebug>
#include "5566.h"
#include "profnext-frontpanel.h"

static QLoggingCategory category("FRONTPANEL");

/**
*  Формирование посылки cpu_cmd_val_t
**/
void ProfnextFrontPanel::cmd_profnext(T_CMD_TYPE type, T_CMD_NUM idx, T_NODE_DYN_ATTR *attr, char *data)
{
        // qCDebug(category) << "cmd_profnext" << idx;

        cpu_cmd_profnext_t cmd_profnext;
        int output_size;
        QByteArray output_data(sizeof(cpu_cmd_profnext_t)*2, 0);

        memset(&cmd_profnext, 0, sizeof(cpu_cmd_profnext_t));
        cmd_profnext.cmd = CPU_CMD_PROFNEXT;
        cmd_profnext.type = type;
        cmd_profnext.idx = idx;
        cmd_profnext.attr_flags = attr->fDisabled;
        
        uint8_t max_val;
        switch (type)
        {
                case TYPE_INDEXED_STRING_P:
                case TYPE_S16_P:
                case TYPE_U16_P:
                        max_val = sizeof(uint16_t);
                        break;
                
                case TYPE_U8_P:
                case TYPE_S8_P:
                case TYPE_STATUS_P:
                case TYPE_STATUS_PX_P:
                        max_val = sizeof(uint8_t);
                        break;

                case TYPE_U32_P:
                case TYPE_S32_P:
                        max_val = sizeof(uint32_t);
                        break;

                case TYPE_U64_P:
                case TYPE_S64_P:
                        max_val = sizeof(uint64_t);
                        break;

                case TYPE_FLOAT_P:
                        max_val = 4;
                        break;

                case TYPE_DOUBLE_P:
                        max_val = 8;
                        break;

                default:
                        max_val = VAL_MAX_SIZE;
                        break;
        }
	
        for (uint8_t i = 0; i < max_val; i ++){
                cmd_profnext.buf[i] = *data;
                data ++;
        }

        output_size = encode_5566((char*)&cmd_profnext, sizeof(cpu_cmd_profnext_t), 
                output_data.data());
        output_data.resize(output_size);
        queue_data(output_data);        //постановка в очередь
}

/**
*  Формирование посылки cpu_ack_t
**/
void ProfnextFrontPanel::cmd_ack()
{
        // qCDebug(category) << "cmd_ack";

        cpu_cmd_ack_t cmd_ack;
        int output_size;
        QByteArray output_data(sizeof(cpu_cmd_ack_t)*2, 0);

        memset(&cmd_ack, 0, sizeof(cpu_cmd_ack_t));
        cmd_ack.cmd = CPU_CMD_ACK;
        cmd_ack.ack = 1;

        output_size = encode_5566((char*)&cmd_ack, sizeof(cpu_cmd_ack_t), 
                output_data.data());
        output_data.resize(output_size);
        queue_data(output_data);        //постановка в очередь
}

/**
*  Формирование посылки cpu_cmd_profnext_t
**/
void ProfnextFrontPanel::parsing_cpu_cmd_profnext(cpu_cmd_profnext_t *cmd_profnext)
{
        switch (cmd_profnext->type)
        {
                case TYPE_INDEXED_STRING_P:
                        emit indexed_string_event(cmd_profnext->idx, (uint16_t)(cmd_profnext->buf[0] | cmd_profnext->buf[1] << 8));
                        break;
                
                default:
                        break;
        }
}

/**
*  Обработка сообщений от XMEGA
**/
void ProfnextFrontPanel::process_msg(QByteArray *msg_data)
{
        cpu_cmd_profnext_t *cmd_profnext = (cpu_cmd_profnext_t *) msg_data->data();
        switch (msg_data->at(0))
        {
                case CPU_CMD_ACK:
                        //реальная проверка ack, после CPU_CMD_ACK должен получить 1
                        if (msg_data->at(1)){
                                msg_in_progress = 0;
                                timeout_timer.stop();
                                send_msg();
                                // qCDebug(category) << "ack recieved";
                        }
                        break;

                case CPU_CMD_PROFNEXT:
                        //обработка сообщений записи переменных от profnext
                        parsing_cpu_cmd_profnext(cmd_profnext);
                        //cmd_ack();
                break;

                default:
                        qCDebug(category) << "Unknown message";
                        break;
        }
}

/**
*  Считывание по протоколу 5566
**/
int ProfnextFrontPanel::read_5566()
{
        int ret = 0;
        int i;
        int remove_size = 0;
        int msg_size;

        if(uart_data.size()<8)
                return 0;

        //если не получили старт
        if((uart_data.at(0)!=SOB)
                &&(uart_data.at(1)!=SOB)){
                uart_data.remove(0, 1);
                return 0;
        }

        //если получили старт (55)
        for(i=0; i<uart_data.size()-1; i++){
                //если получили стоп (66)
                if((uart_data.at(i)==EOB)&&
                        (uart_data.at(i+1)==EOB)){
                                ret = 1;
                                remove_size = i + 2;
                                QByteArray msg_data(i + 2, 0);
                                msg_size = decode_5566(uart_data.data(), msg_data.data(), i + 2);
                                msg_data.resize(msg_size);
                                if(msg_size>0){
                                        process_msg(&msg_data);
                                }
                                break;
                }
                //если получили повторный старт (55) раньше стоп (66), игнорируем все от предыдущего старта
                if((uart_data.at(i)==SOB)&&
                        (uart_data.at(i+1)==SOB)&&(i>0)){
                        ret = 1;
                        remove_size = i;
                }

        }
        uart_data.remove(0, remove_size);
        return ret;
}

/**
*  Поставить сообщение в очередь
**/
void ProfnextFrontPanel::queue_data(QByteArray msg_data)
{
        msg_queue.push_back(msg_data);
        if(msg_in_progress==0)
                send_msg();
}

/**
*  Отправка сообщения
**/
void ProfnextFrontPanel::send_msg()
{
        // qCDebug(category) << "send_msg";
        if(!msg_queue.isEmpty()){
                msg_in_progress = 1;
                QByteArray tmp;
                tmp = msg_queue.takeFirst();
                uart->write(tmp);
                timeout_timer.start(UART_TIMEOUT);
        }
}

/**
*  Обработка таймаута по ответу XMEGA
**/
void ProfnextFrontPanel::uart_timeout()
{
        qCDebug(category) << "Profnext: UART timeout";
        msg_in_progress = 0;
        send_msg();
}

/**
*  Чтение буфера uart
**/
void ProfnextFrontPanel::serial_read()
{
        // qCDebug(category) << "Read";
        uart_data += uart->readAll();
        while(read_5566()){};
}




ProfnextFrontPanel::ProfnextFrontPanel(QString port)
{
        msg_in_progress = 0;
        qCDebug(category) << "Started";
        //open port
        uart = new QSerialPort();
        uart->setPortName(port);
        uart->setBaudRate(QSerialPort::Baud115200);
        if (!uart->open(QIODevice::ReadWrite)) {
                qDebug() << "Failed to open serial port for front panel";
                return;
        }
        connect(uart, &QSerialPort::readyRead, this, &ProfnextFrontPanel::serial_read);

        timeout_timer.setSingleShot(true);
        connect(&timeout_timer, SIGNAL(timeout()), this, SLOT(uart_timeout()));
}

ProfnextFrontPanel::~ProfnextFrontPanel()
{
        //close port
        uart->close();
}
