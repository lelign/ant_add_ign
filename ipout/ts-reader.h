#ifndef TS_READER_H
#define TS_READER_H

#include <QString>
#include <QThread>
#include <QQueue>
#include <QWaitCondition>
#include <QMutex>
#include "ts-utils.h"

class TsReader : public QObject
{
public:
        TsReader();
        int queue_empty_size();
        int queue_full_size();
        data_t get_data(void);
        void return_data(data_t data);
        virtual ~TsReader();
        virtual void input_enable(int enable);
        int get_input_detected();
        void push_data(uint8_t * data_in, int len);
private:
        QQueue<data_t> queue_empty;
        QQueue<data_t> queue_full;
        void queue_allocate();
        void queue_free();
        virtual void after_open() {};
        virtual void before_close() {};
        int input_enabled;
        int input_detected;
        QWaitCondition bufferNotEmpty;
        QMutex mutex;
        data_t data_invalid;
protected:
        void readout();
};

#endif
