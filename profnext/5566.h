#ifndef H_5566_H
#define H_5566_H
#ifdef __cplusplus
extern "C" {
#endif

#define SOB 0x05
#define EOB 0x06
#define ESC 0x04

#define CMDS 0x04
#define CMDE 0x0B

uint16_t get_payload_len_5566(char * buf);
unsigned int encode_5566(char * data, int datalen, char * dst);
int decode_5566(char * data, char * dst, int datalen);
int recieve_5566(int fd, char * buf, int size);

#ifdef __cplusplus
}
#endif
#endif
