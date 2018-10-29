/**
 * @copyright (C) 2017 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "MLX90640_I2C_Driver.h"


void MLX90640_I2CInit()
{   
	MX_I2C1_Init();
}


int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data)
{

	uint8_t* bp = (uint8_t*) data;

    int ack = 0;                               
    int cnt = 0;
    
    ack = HAL_I2C_Mem_Read(&hi2c1, (slaveAddr<<1), startAddress, I2C_MEMADD_SIZE_16BIT, bp, nMemAddressRead*2, 500);

    if (ack != HAL_OK)
    {
        return -1;
    }
    
    //MLX90640 sends data in BIG ENDIAN

    //flip if byte order is LITTLE ENDIAN
#if __BYTE_ORDER == __LITTLE_ENDIAN
    for(cnt=0; cnt < nMemAddressRead*2; cnt+=2) {
    	uint8_t tmpbytelsb = bp[cnt+1];
    	bp[cnt+1] = bp[cnt];
    	bp[cnt] = tmpbytelsb;
    }
#endif

    
    return 0;   
} 

//enum freq: 0 = 400khz, 1=1mhz
void MLX90640_I2CFreqSet(I2C_Frequency f)
{

	//check if instance is not in use with 100ms
	long curTick = HAL_GetTick();
	while(HAL_GetTick() - curTick <= 100){
		if(hi2c1.State == HAL_I2C_STATE_READY){
			if(f == I2C_HZ_400k){
				hi2c1.Instance->TIMINGR = 0x00201953 & 0xF0FFFFFFU;
			}else if(f == I2C_HZ_1M){
				hi2c1.Instance->TIMINGR = 0x00200B19 & 0xF0FFFFFFU;
			}
			break;
		}
	}


}

int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data)
{

    uint8_t sa;
    int ack = 0;
    uint8_t cmd[2];
    static uint16_t dataCheck;

    sa = (slaveAddr << 1);

#if __BYTE_ORDER == __LITTLE_ENDIAN
    cmd[0] = data >> 8;
    cmd[1] = data & 0x00FF;
#else
    cmd[0] = data & 0x00ff;
    cmd[1] = data>>8;
#endif

    ack = HAL_I2C_Mem_Write(&hi2c1, sa, writeAddress, I2C_MEMADD_SIZE_16BIT, cmd, sizeof(cmd), 500);

    if (ack != HAL_OK)
    {
        return -1;
    }         
    
    MLX90640_I2CRead(slaveAddr,writeAddress,1, &dataCheck);
    
    if ( dataCheck != data)
    {
        return -2;
    }    
    
    return 0;
}

