#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include "stdint.h"
#include "string.h"

#define MAX_CMD_DATA_LENGTH 10

#define CMD_HEADER_SIZE 3

//commandCode
#define CMD_NO_COMMAND 0xFF

#define CMD_PING 				0x00
#define CMD_DUMP_EE 			0x01
#define CMD_GET_FRAME_DATA 		0x02
#define CMD_SET_RESOLUTION 		0x03
#define CMD_GET_CUR_RESOLUTION 	0x04
#define CMD_SET_REFRESH_RATE 	0x05
#define CMD_GET_REFRESH_RATE 	0x06
#define CMD_SET_MODE 			0x07
#define CMD_GET_CUR_MODE 		0x08
#define CMD_SET_AUTO_FRAME_DATA_SENDING	0x09

#define COMMAND_STRUCT_SIZE 3
typedef struct {
	uint8_t commandCode;
	uint16_t dataLength;
	uint8_t data[MAX_CMD_DATA_LENGTH];
} cmd_struct;


//responseCode
#define RSP_PING  				0x00
#define RSP_DUMP_EE 			0x01
#define RSP_GET_FRAME_DATA 		0x02
#define RSP_SET_RESOLUTION 		0x03
#define RSP_GET_CUR_RESOLUTION 	0x04
#define RSP_SET_REFRESH_RATE 	0x05
#define RSP_GET_REFRESH_RATE 	0x06
#define RSP_SET_MODE 			0x07
#define RSP_GET_CUR_MODE 		0x08
#define RSP_SET_AUTO_FRAME_DATA_SENDING 0x09

//dataCode
#define CODE_OK 						0
#define CODE_NACK 						-1
#define CODE_WRITTEN_VALUE_NOT_SAME 	-2
#define CODE_I2C_FREQ_TOO_LOW 			-8

#define RESPONSE_STRUCT_SIZE 4

typedef struct {
	uint8_t responseCode;
	int8_t dataCode;
	uint16_t dataLength;
	uint8_t* data;
} rsp_struct;


cmd_struct parse_command();
void write_response_to_buffer(rsp_struct rsp, uint8_t* buf);


#endif /* _PROTOCOL_H_ */
