#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/poll.h>
#include <stdint.h>
#include "5566.h"

uint16_t get_payload_len_5566(char * buf)
{
        uint16_t datalen;

        datalen = (buf[2]&0x7f) | (buf[3]&0x7f)<<7;
        return datalen;
}

unsigned int encode_5566(char * data, int datalen, char * dst)
{
        int i;
        int result_len = 0;
        unsigned int crc=0;
        
        *(dst++)=SOB;
        *(dst++)=SOB;
        *(dst++)=((datalen) % 0x80)|0x80;
        *(dst++)=((datalen) / 0x80)|0x80;
        crc += ((datalen) % 0x80)|0x80;
        crc += ((datalen) / 0x80)|0x80;
        for (i=0; i<datalen; i++){
                if((data[i]>=CMDS)&&(data[i]<=CMDE)){
                        *(dst++)=ESC;
                        *(dst++)=data[i]+0x08;
                        crc += ESC;
                        crc += data[i]+0x08;
                        result_len += 1;
                }
                else{
                        *(dst++)=data[i];
                        crc += data[i];
                }
                result_len += 1;
        }
        *(dst++)=(unsigned char) (-crc|0x80);
        *(dst++)=(unsigned char) (((-crc)>>7)|0x80);
        *(dst++)=EOB;
        *(dst++)=EOB;
        result_len += 8+1;
        return result_len;
}

int decode_5566(char * data, char * dst, int datalen)
{
        int i;
        int crc=0;
        unsigned char crc1, crc2;
        char esc_flag=0;
        unsigned int ret=0;

        for (i=2; i<datalen-4; i++){
                crc += data[i];
                
        }
        crc1 = (unsigned char) (-crc|0x80);
        crc2 = (unsigned char) (((-crc)>>7)|0x80);
        if((crc1!=data[datalen-4])&&(crc2!=data[datalen-3])){
                printf("crc error\n");
                return 0;
        }
        for(i=4; i<datalen-4; i++){
                if(data[i]==ESC) esc_flag=1;
                else{
                        if(esc_flag){
                                *(dst++) = data[i]-0x08;
                                esc_flag=0;
                        }
                        else{
                                *(dst++) = data[i];
                        }
                        ret++;
                }
        }
        return ret;
}
