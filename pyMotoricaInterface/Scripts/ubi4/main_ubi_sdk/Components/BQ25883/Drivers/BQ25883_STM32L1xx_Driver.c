/*
 * BQ25883_Driver.c
 *
 *  Created on: Jun 18, 2023
 *      Author: PC
 */

#include "BQ25883_Driver.h"

#ifdef ENABLE_BQ25883

I2C_HandleTypeDef* i2c_inst;


void BQ25883_Driver_Init(I2C_HandleTypeDef* i2c)
{
	i2c_inst = i2c;
}

BQ25883_I2C_STATUS_ENUM BQ25883_Read(uint8_t Address, uint8_t Register, uint8_t* Data, uint8_t size)
{
	  switch(HAL_I2C_Mem_Read(i2c_inst,Address,Register,1,Data,size,100))
	  {
	  case HAL_OK:
		  return BQ25883_I2C_STATUS_OK;
	  case HAL_ERROR:
	  		  return BQ25883_I2C_STATUS_ERROR;
	  case HAL_BUSY:
	  		  return BQ25883_I2C_STATUS_BUSY;
	  case HAL_TIMEOUT:
	  		  return BQ25883_I2C_STATUS_TIMEOUT;
	  }
}


BQ25883_I2C_STATUS_ENUM BQ25883_Write(uint8_t Address, uint8_t Register, uint8_t* Data, uint8_t size)
{

	switch(HAL_I2C_Mem_Write(i2c_inst,Address,Register,1,Data,size,100))
	{
	case HAL_OK:
	  return BQ25883_I2C_STATUS_OK;
	case HAL_ERROR:
		  return BQ25883_I2C_STATUS_ERROR;
	case HAL_BUSY:
		  return BQ25883_I2C_STATUS_BUSY;
	case HAL_TIMEOUT:
		  return BQ25883_I2C_STATUS_TIMEOUT;
	}
}

void BQ25883_Dalay(uint32_t time_ms)
{
	HAL_Delay(time_ms);
}

#endif /* ENABLE_BQ25883 */
