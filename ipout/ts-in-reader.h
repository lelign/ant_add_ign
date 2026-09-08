#ifndef TS_IN_READER_H
#define TS_IN_READER_H

#include "ts-reader.h"
#include <QMutex>

class TsInReader : public TsReader, public QThread
{
public:
        TsInReader(const char * fname);
        void run() override;
        void stop();

private:
        const char * fname;
        int thread_exit;
        int f_ts;
protected:
        void readout();
};

#endif
