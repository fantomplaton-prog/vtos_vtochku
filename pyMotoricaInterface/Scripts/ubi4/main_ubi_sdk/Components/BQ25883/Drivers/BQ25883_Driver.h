/*
  ******************************************************************************
  * @file    BQ25883_Driver.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef SRC_BQ25883_DRIVER_H_
#define SRC_BQ25883_DRIVER_H_

#include "UBI_SDK.h"
#include "HAL_DRIVERS.h"
#include "stdint.h"
#include "stdbool.h"

#ifdef ENABLE_BQ25883

typedef enum
{
	BQ25883_I2C_STATUS_OK		= 0x00,
	BQ25883_I2C_STATUS_ERROR	= 0x01,
	BQ25883_I2C_STATUS_BUSY		= 0x02,
	BQ25883_I2C_STATUS_TIMEOUT	= 0x03

}BQ25883_I2C_STATUS_ENUM;

void BQ25883_Driver_Init(I2C_HandleTypeDef* i2c);

BQ25883_I2C_STATUS_ENUM BQ25883_Read(uint8_t Address, uint8_t Register, uint8_t* Data, uint8_t size);

BQ25883_I2C_STATUS_ENUM BQ25883_Write(uint8_t Address, uint8_t Register, uint8_t* Data, uint8_t size);

void BQ25883_Dalay(uint32_t time_ms);

#endif /* ENABLE_BQ25883 */

#endif /* SRC_BQ25883_DRIVER_H_ */
