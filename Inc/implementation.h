#ifndef _IMPLEMENTATION_H_
#define _IMPLEMENTATION_H_

#include "protocol.h"
#include "MLX90640_API.h"


#define MLX90640_SLAVE_ADDRESS 0x33

void write_big_endian(void* dst_buffer, void* src_buffer, int src_len);
void main_loop();
void execute_command(cmd_struct command);
#endif
