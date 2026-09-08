#ifndef TS_UTILS_H
#define TS_UTILS_H

#include <stdint.h>

#define MAX_TS (0x7FFFFFF)

typedef struct  {
        uint32_t size;
        char * buf;
}data_t;

uint32_t extract_ts(char * block, int size);
int32_t ts_diff(uint32_t t1, uint32_t t2);

#endif