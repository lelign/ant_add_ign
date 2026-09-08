#include "ts-utils.h"

uint32_t extract_ts(char * block, int size)
{
        uint32_t ret;

        ret = block[0] << 24;
        ret |= block[1] << 16;
        ret |= block[2] << 8;
        ret |= block[3] << 0;

        return ret & MAX_TS;
}

int32_t ts_diff(uint32_t t1, uint32_t t2)
{
        int32_t d;

        d = (int32_t) t1 - (int32_t) t2;
        if(d > MAX_TS/2)
                d -= MAX_TS;
        if(d < -MAX_TS/2)
                d += MAX_TS;
        return d;
}
