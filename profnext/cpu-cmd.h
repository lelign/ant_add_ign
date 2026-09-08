#ifndef CPU_CMD_H
#define CPU_CMD_H

#include <stdint.h>

#define VAL_MAX_SIZE    32

#define PERIPH_STATUS_OK				1
#define PERIPH_STATUS_ERROR				2

typedef	enum {
	TYPE_STATUS_P,          // profnext status
	TYPE_STATUS_PX_P,       // proflex status
	TYPE_U8_P,              // uint8
	TYPE_S8_P,              // int8
	TYPE_U16_P,             // uint16
	TYPE_S16_P,             // int 16
	TYPE_U32_P,             // uint32
	TYPE_S32_P,             // int32
	TYPE_U64_P,             // uint64
	TYPE_S64_P,             // int64
	TYPE_FLOAT_P,           // (4 - byte floating point)
	TYPE_DOUBLE_P,          // (8 - byte floating point)
	TYPE_STRING_P,          // sting ended zero, max 255 characters
	TYPE_INDEXED_STRING_P,
	TYPE_DATA_ARRAY_P,
	TYPE_ALARM_P,
} T_CMD_TYPE;

typedef enum {
	STATUS_IDX,
	STATUS_PX_IDX,
	CMD_0,
	CMD_1,
	CMD_2,
	CMD_3,
	CMD_4,
	CMD_5,
	CMD_6,
	CMD_7,
} T_CMD_NUM;

typedef struct _t_node_attr {
	uint8_t fDisabled	 : 1;//недоступен для изменения
} T_NODE_DYN_ATTR;

typedef struct __attribute__((__packed__)) {
	uint8_t cmd;
	uint8_t ack;
}cpu_cmd_ack_t;

typedef struct __attribute__((__packed__)) {
	uint8_t cmd;			//тип команды для XMEGA
    uint8_t type;			//тип регулиорвки, см. T_CMD_TYPE
    uint8_t idx;			//номер регулировки
	uint8_t attr_flags;		//динамические атрибуты (пока только атрибут видимости), если есть, иначе - 0
	char buf[VAL_MAX_SIZE];
}cpu_cmd_profnext_t;

enum {
CPU_CMD_PROFNEXT, 
CPU_CMD_ACK,
};

enum prolex_header_enum{
	REF_POS = 0,
	INPUT_POS,
	FREEZE_POS,
	BARS_POS
};

#endif