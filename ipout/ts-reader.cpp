/*
 * Модуль чтения из устройства ввода Mpeg-ts
 * 2017 Vladimir Iakovlev <nagos@inbox.ru>
*/
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <QDebug>
#include "ts-reader.h"

#define QUEUE_SIZE (1024)
#define BUF_SIZE (16*1024)

TsReader::TsReader() :
        input_enabled(1),
        input_detected(0)
{
        queue_allocate();
        data_invalid.size = 0;
        data_invalid.buf = 0;
}

TsReader::~TsReader()
{
        queue_free();
}

data_t TsReader::get_data(void)
{
        data_t data;
        bool wait_satus=true;
        mutex.lock();
        if(queue_full.isEmpty()){
                wait_satus = bufferNotEmpty.wait(&mutex, 1000);
        }
        if(wait_satus){
                data = queue_full.dequeue();
                mutex.unlock();
                return data;
        }
        else{
                mutex.unlock();
                return data_invalid;
        }
}

void TsReader::return_data(data_t data)
{
        Q_CHECK_PTR(data.buf);
        mutex.lock();
        queue_empty.enqueue(data);
        mutex.unlock();
}

void TsReader::queue_allocate()
{
        char * buf;
        data_t data;

        for(int i=0; i<QUEUE_SIZE; i++){
                buf = (char *) malloc(BUF_SIZE);
                data.buf = buf;
                queue_empty.enqueue(data);
        }
}

void TsReader::queue_free()
{
        data_t buf;

        while (!queue_empty.isEmpty()){
                buf = queue_empty.dequeue();
                Q_CHECK_PTR(buf.buf);
                free(buf.buf);
        }

        while (!queue_full.isEmpty()){
                buf = queue_full.dequeue();
                Q_CHECK_PTR(buf.buf);
                free(buf.buf);
        }
}

void TsReader::readout()
{
}

void TsReader::push_data(uint8_t * data_in, int len)
{
        int skip_data = 0;
        data_t data;

        if(!input_enabled)
                return;
        if(len==0)
                return;
        // получить свободный буфер
        mutex.lock();
        if((!queue_empty.isEmpty()) && input_enabled){
                data = queue_empty.dequeue();
                skip_data = 0;
                memcpy(data.buf, data_in, len);
                data.size = len;
        }
        else {
                skip_data = 1;
        }
        mutex.unlock();


        // отдать заполненый буфер
        mutex.lock();
        if(!skip_data){
                Q_CHECK_PTR(data.buf);
                queue_full.enqueue(data);
                bufferNotEmpty.wakeAll();
        }
        mutex.unlock();
}

void TsReader::input_enable(int enable)
{
        mutex.lock();
        input_enabled = enable;
        mutex.unlock();
}

int TsReader::get_input_detected()
{
        return input_detected;
}

int TsReader::queue_empty_size()
{
        return queue_empty.size();
}

int TsReader::queue_full_size()
{
        return queue_full.size();
}
