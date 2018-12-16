#include "implementation.h"
#include "usbd_cdc_if.h"
#include "cobs.h"

#include "tim.h"

#include "Version.h"

//globals
//auto send var
uint8_t auto_frame_data_sending = 0;

enum IndicatorLed {
		BLUE_LED, RED_LED
};


//Max dutyCycle value = 1024
void setPwm(enum IndicatorLed led, uint16_t dutyCycle){

	static TIM_OC_InitTypeDef sConfigOC = {
			.OCMode = TIM_OCMODE_PWM1,
			.Pulse = 0,
			.OCPolarity = TIM_OCPOLARITY_HIGH,
			.OCFastMode = TIM_OCFAST_DISABLE,
			.OCNPolarity = TIM_OCNPOLARITY_HIGH,
			.OCNIdleState = TIM_OCNIDLESTATE_RESET
	};
	sConfigOC.Pulse = dutyCycle;

	if(led == BLUE_LED){
		HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);
		HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	}else if(led == RED_LED){
		HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2);
		HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
	}

}

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
	response.data = NULL;
	//pointer to buffer location for transmission
	uint8_t* pointer_to_tx_buffer_offset_rsp_begin = CDC_GetTransmitBuffer() + 100;

	//offset by 4 bytes (response header) +1B initial COBS encode overhead
	uint8_t* pointer_to_buffer_offset_data = pointer_to_tx_buffer_offset_rsp_begin + RESPONSE_STRUCT_SIZE;

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
			response.data = NULL;
			response.dataCode = MLX90640_DumpEE(MLX90640_SLAVE_ADDRESS, (uint16_t*) pointer_to_buffer_offset_data);
			response.dataLength = 832*2;

			//swap to BIG ENDIAN
			for(int i = 0; i< 832*2; i+=2){
				uint8_t tmp = pointer_to_buffer_offset_data[i+1];
				pointer_to_buffer_offset_data[i+1] = pointer_to_buffer_offset_data[i];
				pointer_to_buffer_offset_data[i] = tmp;
			}
		break;

		case CMD_GET_FRAME_DATA:
			response.responseCode = RSP_GET_FRAME_DATA;
			response.data = NULL;
			response.dataCode = MLX90640_GetFrameData(MLX90640_SLAVE_ADDRESS, (uint16_t*) pointer_to_buffer_offset_data);
			response.dataLength = 834*2;

			//swap to BIG ENDIAN
			for(int i = 0; i< 834*2; i+=2){
				uint8_t tmp = pointer_to_buffer_offset_data[i+1];
				pointer_to_buffer_offset_data[i+1] = pointer_to_buffer_offset_data[i];
				pointer_to_buffer_offset_data[i] = tmp;
			}
		break;

		case CMD_SET_RESOLUTION:
			response.responseCode = RSP_SET_RESOLUTION;
			response.data = NULL;
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
			response.data = NULL;
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
			response.data = NULL;
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

		case CMD_GET_FIRMWARE_VERSION :
			response.responseCode = RSP_GET_FIRMWARE_VERSION;
			response.dataLength = sizeof(struct FirmwareVersion);
			response.data = malloc(sizeof(struct FirmwareVersion));
			response.dataCode = 0;

			write_big_endian(response.data+0, &firmwareVersion.major, sizeof(int));
			write_big_endian(response.data+sizeof(int), &firmwareVersion.minor, sizeof(int));
			write_big_endian(response.data+sizeof(int)*2, &firmwareVersion.revision, sizeof(int));

		break;

	}	

	//+1B COBS OVERHEAD initial byte
	write_response_to_buffer(response, pointer_to_tx_buffer_offset_rsp_begin);
	if(response.data != NULL) {
		free(response.data);
	}

	//non-encoded data sits in transmit buffer with first byte free for COBS encoding overhead byte
	//lets begin with COBS encoding
	//TODO cobs encoding with same source and destination does not always work, investigate further...
	cobs_encode_result encode_result = cobs_encode(CDC_GetTransmitBuffer(), CDC_GetTransmitBufferSize(), pointer_to_tx_buffer_offset_rsp_begin, RESPONSE_STRUCT_SIZE + response.dataLength);

	if(encode_result.status == COBS_ENCODE_OK){
		//successfully encoded, finally add a message delimiter (0x00) at the end of COBS data
		CDC_GetTransmitBuffer()[encode_result.out_len] = 0x00;

		//lets transmit this message. COBS data (out_len) + 1B (Message Delimiter)
		CDC_Transmit_FS(CDC_GetTransmitBuffer(), encode_result.out_len + 1);
	}


	while(CDC_Transmit_Complete() != USBD_OK){
		//wait for usb to transfer...
	}


}


void write_big_endian(void* dst_buffer, void* src_buffer, int src_len){
	for(int i = 0; i < src_len; i++){
		((uint8_t*) (dst_buffer))[i] = ((uint8_t*) src_buffer)[(src_len-1) - i];
	}
}

