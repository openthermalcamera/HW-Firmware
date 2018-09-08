#include "protocol.h"
#include "usbd_cdc_if.h"

static int serial_get_available_bytes(){
	return CDC_Available();
}

static uint8_t serial_get_byte(){
	return CDC_Read();
}

static void serial_write_byte(){

}

static void write_data_length(uint16_t dataLength){

#if __BIG_ENDIAN__
	for(int i = 0; i<2; i++){
		serial_write_byte( ((uint8_t*) &dataLength)[i] );
	}
#else
	for(int i = 1; i >= 0; i--){
		serial_write_byte( ((uint8_t*) &dataLength)[i] );
	}
#endif

}


static uint16_t read_data_length(){
	uint16_t r = 0;

#if __BIG_ENDIAN__
	for(int i = 0; i<2; i++){
		((uint8_t*) &r)[i] = serial_get_byte();
	}
#else
	for(int i = 1; i >= 0; i--){
		((uint8_t*) &r)[i] = serial_get_byte();
	}
#endif

	return r;
}

static uint8_t peek_cmd_code(){
	return CDC_Peak(0);
}

static uint16_t peek_data_length(){
	uint16_t r = 0;

#if __BIG_ENDIAN__
	((uint8_t*) &r)[0] = CDC_Peak(1);
	((uint8_t*) &r)[1] = CDC_Peak(2);
#else
	((uint8_t*) &r)[0] = CDC_Peak(2);
	((uint8_t*) &r)[1] = CDC_Peak(1);
#endif

	return r;

}


cmd_struct parse_command(){

	cmd_struct command;

	//if enough data to start parsing
	if(serial_get_available_bytes() >= 3){
		command.commandCode = peek_cmd_code();
		command.dataLength = peek_data_length();
		if(command.dataLength <= MAX_CMD_DATA_LENGTH && CDC_Available() + 3 >= command.dataLength){
			//read
			command.commandCode = serial_get_byte();
			command.dataLength = read_data_length();

			//seq
			for(int i = 0; i<command.dataLength; i++){
				command.data[i] = serial_get_byte();
			}
		}else{
			//not enough data
			command.commandCode = 255;
			return command;
		}

	}else{
		//not enough data
		command.commandCode = 255;
		return command;
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

void send_response(rsp_struct response){


	serial_write_byte(response.responseCode);
	serial_write_byte(response.dataCode);
	write_data_length(response.dataLength);

	//seq write data
	for(int i = 0; i<response.dataLength; i++){
		serial_write_byte(response.data[i]);
	}

}
