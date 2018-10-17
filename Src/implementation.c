#include "implementation.h"
#include "usbd_cdc_if.h"
#include "cobs.h"


//globals
//auto send var
uint8_t auto_frame_data_sending = 0;

void main_loop(){


	//main loop, never break out
	while(1){

		cmd_struct cmd = parse_command();
		if(cmd.commandCode != 255){
			execute_command(cmd);
		}

		if(auto_frame_data_sending == 0x01){
			//if new frame available, send it out immediately
			if(MLX90640_IsFrameDataAvailable()){
				cmd_struct fake_get_frame_data_cmd;
				fake_get_frame_data_cmd.commandCode = CMD_GET_FRAME_DATA;
				fake_get_frame_data_cmd.dataLength = 0;
				execute_command(fake_get_frame_data_cmd);
			}
		}

	}


}



void execute_command(cmd_struct command){
	rsp_struct response;
	response.data = 0;
	//offset by 4 bytes (response header) +1B initial COBS encode overhead
	uint8_t* pointer_to_tx_buffer_offset = CDC_GetTransmitBuffer() + RESPONSE_STRUCT_SIZE + 1;

	while(CDC_Transmit_Complete() != USBD_OK){
		//wait for usb to transfer...
	}

	switch(command.commandCode){
		case CMD_PING :
			response.responseCode = RSP_PING;
			response.dataCode = 0;
			response.dataLength = 1;
			response.data = malloc(1);
			response.data[0] = command.data[0] * 2;
		break;

		case CMD_DUMP_EE:
			response.responseCode = RSP_DUMP_EE;
			response.data = 0;
			response.dataCode = MLX90640_DumpEE(MLX90640_SLAVE_ADDRESS, (uint16_t*) pointer_to_tx_buffer_offset);
			response.dataLength = 832*2;

			//swap to BIG ENDIAN
			for(int i = 0; i< 832*2; i+=2){
				uint8_t tmp = pointer_to_tx_buffer_offset[i+1];
				pointer_to_tx_buffer_offset[i+1] = pointer_to_tx_buffer_offset[i];
				pointer_to_tx_buffer_offset[i] = tmp;
			}
		break;

		case CMD_GET_FRAME_DATA:
			response.responseCode = RSP_GET_FRAME_DATA;
			response.data = 0;
			response.dataCode = MLX90640_GetFrameData(MLX90640_SLAVE_ADDRESS, (uint16_t*) pointer_to_tx_buffer_offset);
			response.dataLength = 834*2 ;

			//swap to BIG ENDIAN
			for(int i = 0; i< 834*2; i+=2){
				uint8_t tmp = pointer_to_tx_buffer_offset[i+1];
				pointer_to_tx_buffer_offset[i+1] = pointer_to_tx_buffer_offset[i];
				pointer_to_tx_buffer_offset[i] = tmp;
			}
		break;

		case CMD_SET_RESOLUTION:
			response.responseCode = RSP_SET_RESOLUTION;
			response.data = 0;
			response.dataLength = 0;
			response.dataCode = MLX90640_SetResolution(MLX90640_SLAVE_ADDRESS, command.data[0]);
		break;

		case CMD_GET_CUR_RESOLUTION :
			response.responseCode = RSP_GET_CUR_RESOLUTION;
			response.dataLength = 1;
			response.data = malloc(1);
			response.dataCode = MLX90640_GetCurResolution(MLX90640_SLAVE_ADDRESS);
			response.data[0] = response.dataCode;
		break;

		case CMD_SET_REFRESH_RATE :
			response.responseCode = RSP_SET_REFRESH_RATE;
			response.dataLength = 0;
			response.data = 0;
			response.dataCode = MLX90640_SetRefreshRate(MLX90640_SLAVE_ADDRESS, command.data[0]);
		break;

		case CMD_GET_REFRESH_RATE :
			response.responseCode = RSP_GET_REFRESH_RATE;
			response.dataLength = 1;
			response.data = malloc(1);
			response.dataCode = MLX90640_GetRefreshRate(MLX90640_SLAVE_ADDRESS);
			response.data[0] = response.dataCode;
		break;

		case CMD_SET_MODE :
			response.responseCode = RSP_SET_MODE;
			response.dataLength = 0;
			response.data = 0;
			if(command.data[0] == 0){
				response.dataCode = MLX90640_SetInterleavedMode(MLX90640_SLAVE_ADDRESS);
			}else if(command.data[0] == 1){
				response.dataCode = MLX90640_SetChessMode(MLX90640_SLAVE_ADDRESS);
			}else{
				//error
				response.dataCode = -3;
			}
		break;

		case CMD_GET_CUR_MODE:
			response.responseCode = RSP_GET_CUR_MODE;
			response.dataLength = 1;
			response.data = malloc(1);
			response.dataCode = MLX90640_GetCurMode(MLX90640_SLAVE_ADDRESS);
			response.data[0] = response.dataCode;
		break;

		case CMD_SET_AUTO_FRAME_DATA_SENDING:
			response.responseCode = RSP_SET_AUTO_FRAME_DATA_SENDING;
			response.dataLength = 1;
			response.data = malloc(1);
			response.dataCode = 0;
			response.data[0] = auto_frame_data_sending;

			auto_frame_data_sending = command.data[0];
		break;

	}	

	//+1B COBS OVERHEAD initial byte
	write_response_to_buffer(response, CDC_GetTransmitBuffer() + 1);
	if(response.data != 0) {
		free(response.data);
	}

	//non-encoded data sits in transmit buffer with first byte free for COBS encoding overhead byte
	//lets begin with COBS encoding
	cobs_encode_result encode_result = cobs_encode(CDC_GetTransmitBuffer(), CDC_GetTransmitBufferSize(), CDC_GetTransmitBuffer() + 1, RESPONSE_STRUCT_SIZE + response.dataLength);

	if(encode_result.status == COBS_ENCODE_OK){
		//successfully encoded, finally add a message delimiter (0x00) at the end of COBS data
		CDC_GetTransmitBuffer()[encode_result.out_len] = 0x00;

		//lets transmit this message. COBS data (out_len) + 1B
		CDC_Transmit_FS(CDC_GetTransmitBuffer(), encode_result.out_len + 1);
	}


	while(CDC_Transmit_Complete() != USBD_OK){
		//wait for usb to transfer...
	}

}
