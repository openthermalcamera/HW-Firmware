#include "protocol.h"
#include "usbd_cdc_if.h"
#include "cobs.h"

cmd_struct parse_command(){

	cmd_struct command;

	//default set as no command
	command.commandCode = CMD_NO_COMMAND;
	command.dataLength = 0;

	if(CDC_Available() <= 0) return command;

	//check if whole message arrived -> {cmd}, 0x00
	int cmd_length = -1;
	uint8_t cobs_encoded_buffer[MAX_CMD_DATA_LENGTH];

	for(int i = 0; i < CDC_Available(); i++){
		uint8_t peak_byte = CDC_Peak(i);
		if(peak_byte == 0x00){
			cmd_length = i;
		}
	}

	if(cmd_length > 0){

		for(int i = 0; i<cmd_length; i++){
			cobs_encoded_buffer[i] = CDC_Read();
		}
		//discard the message end delimiter (0x00)
		CDC_Read();

		//create buffer of suitable size for decoded message
		int cmd_buffer_size = COBS_DECODE_DST_BUF_LEN_MAX(cmd_length);
		uint8_t cmd[cmd_buffer_size];

		//the cobs encoded message is now stored in buffer, decode the message
		cobs_decode_result decode_result = cobs_decode(cmd, cmd_buffer_size, cobs_encoded_buffer, cmd_length);

		//check if result was successful
		if(decode_result.status == COBS_DECODE_OK){
			//start parsing the cmd

			//first check if command is complete (amount of data and data length match)
			if(decode_result.out_len >= CMD_HEADER_SIZE && decode_result.out_len <= MAX_CMD_DATA_LENGTH + CMD_HEADER_SIZE) {
				command.dataLength = (((uint16_t)cmd[1]) << 8) | ((uint16_t) cmd[2]);
				if(command.dataLength + CMD_HEADER_SIZE == decode_result.out_len){
					//command is complete, start parsing fully
					command.commandCode = cmd[0];
					//copy data to command struct
					memcpy(command.data, cmd + CMD_HEADER_SIZE, command.dataLength);

				}

			}

		}

	}


	return command;

}

void write_response_to_buffer(rsp_struct rsp, uint8_t* buf){
	buf[0] = rsp.responseCode;
	buf[1] = rsp.dataCode;

#if __BIG_ENDIAN__
	buf[2] = ((uint8_t*) &rsp.dataLength)[0];
	buf[3] = ((uint8_t*) &rsp.dataLength)[1];
#else
	buf[2] = ((uint8_t*) &rsp.dataLength)[1];
	buf[3] = ((uint8_t*) &rsp.dataLength)[0];
#endif

	if(rsp.data != 0 && rsp.dataLength != 0){
		memcpy(buf+4, rsp.data, rsp.dataLength);
	}
}

