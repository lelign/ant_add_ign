#include <QDebug>
#include <QLoggingCategory>
#include "anc-reader.h"

#define READOUT_CNT (256)
#define S2010_DID (0x41)
#define S2010_SDID (0x07)

#define OP47_DID (0x43)
#define OP47_SDID (0x02)
#define OP47_SIZE (51)

// SMPTE2010
// SCTE104

static QLoggingCategory category("ANC");

AncReader::AncReader(const char * in_fname, QObject *parent)
:
        QThread(parent),
        thread_exit(0),
        readout_cnt(READOUT_CNT),
        ts_reader(new TsInReader(in_fname))
{
        ts_reader->start();
}

void AncReader::run()
{
        data_t data;

        while(thread_exit==0){
                data = ts_reader->get_data();
                if(data.size>0){
                        process(data);
                        ts_reader->return_data(data);
                }
        }
}

void AncReader::stop()
{
        thread_exit = 1;
        ts_reader->stop();
        ts_reader->wait();
        this->wait();
}

void AncReader::process(data_t data)
{
        if(readout_cnt){
                readout_cnt = readout_cnt - 1;
                return;
        }

        uint8_t did;
        uint16_t sdid;
        uint8_t channel;
        uint8_t size;

        channel = data.buf[0];
        did = data.buf[1];
        sdid = data.buf[2];
        size = data.buf[3];

        if(channel < 16){
                if(size != data.size-1-4){
                        //qCDebug(category) << "Incorrect ANC size"; // ign
                        return;
                }                
                if(did==S2010_DID && sdid==S2010_SDID){
                        QByteArray ret = QByteArray(data.buf+1, data.size-1);
                        emit scte_104_data(channel, ret);
                } else if(did==OP47_DID && sdid==OP47_SDID){
                        QByteArray ret = QByteArray(data.buf+1, data.size-1);
                        emit op47_data(channel, ret);
                }
        }else{
                QByteArray ret = QByteArray(data.buf+1, data.size-1);
                emit op42_data(0, ret);
        }
}
