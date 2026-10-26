/*
  ******************************************************************************
  * @file    BQ25883.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef SRC_BQ25883_H_
#define SRC_BQ25883_H_

#include "bq2588x_reg.h"
#include "stdint.h"
#include "stdbool.h"
#include "Drivers/BQ25883_Driver.h"

#ifdef ENABLE_BQ25883

#define BQ25883_WRITE_ADDRESS 	0xD7
#define BQ25883_READ_ADDRESS 	0xD7



typedef BQ25883_I2C_STATUS_ENUM (*write_i2c_bq25883)(uint8_t Address, uint8_t Register, uint8_t* Data, uint8_t size);
typedef BQ25883_I2C_STATUS_ENUM (*read_i2c_bq25883)(uint8_t Address, uint8_t Register, uint8_t* Data, uint8_t size);



typedef struct
{
	write_i2c_bq25883 write_func;
	read_i2c_bq25883 read_func;
}BQ25883_init_struct;


typedef struct
{
	uint8_t VINDPM:			5;
	uint8_t PFM_OOA_DIS: 	1;
	uint8_t EN_BAT_DISCHG: 	1;
	uint8_t EN_VINDPM_RST: 	1;
	uint16_t MinVoltage_mV;
}__attribute__((__packed__)) InputVoltageLimitRegisterMask;

typedef struct
{
	uint8_t IINDPM:			5;
	uint8_t EN_ICO: 		1;
	uint8_t FORCE_INDET: 	1;
	uint8_t FORCE_ICO: 		1;
}__attribute__((__packed__)) InputCurrentLimitRegisterMask;


typedef enum
{
	TREG_60C	= 0x00,
	TREG_80C	= 0x01,
	TREG_100C	= 0x02,
	TREG_120C	= 0x03
}ThermalRegulationThresholdEnum;

typedef enum
{
	VRECHG_100_MV	= 0x00,
	VRECHG_200_MV	= 0x01,
	VRECHG_300_MV	= 0x02,
	VRECHG_400_MV	= 0x03
}BatteryRechargeThresholdOffsetEnum;

typedef struct
{
	uint8_t VRECHG:			2;
	uint8_t BATLOWV:		1;
	uint8_t EN_CHG:			1;
	uint8_t TREG: 			2;
	uint8_t AUTO_INDET_EN: 	1;
	uint8_t EN_OTG: 		1;
}__attribute__((__packed__)) ChargerControl2RegisterMask;


typedef enum
{
	CONTINUOUS_CONVERSION	= 0x00,
	ONE_SHOT_CONVERSION		= 0x01
}ADCModeEnum;

typedef enum
{
	RESOLUTION_15_BIT	= 0x00,
	RESOLUTION_14_BIT	= 0x01,
	RESOLUTION_13_BIT	= 0x02,
	RESOLUTION_12_BIT	= 0x03
}ADCResolutionEnum;

typedef struct
{
	uint8_t RESERVED:		4;
	uint8_t ADC_SAMPLE: 	2;
	uint8_t ADC_RATE:	 	1;
	uint8_t ADC_EN: 		1;
}__attribute__((__packed__)) ADCControlRegisterMask;

typedef struct
{
	uint8_t TDIE_ADC_DIS: 		1;
	uint8_t RESERVED:			1;
	uint8_t TS_ADC_DIS: 		1;
	uint8_t VSYS_ADC_DIS: 		1;
	uint8_t VBAT_ADC_DIS: 		1;
	uint8_t VBUS_ADC_DIS: 		1;
	uint8_t ICHG_ADC_DIS: 		1;
	uint8_t IBUS_ADC_DIS: 		1;
}__attribute__((__packed__)) ADCFunctionDisableRegisterMask;


typedef enum
{
	CSE_NOT_CHARGING 			= 0x00,
	CSE_TRICKLE_CHARGE 			= 0x01,
	CSE_PRE_CHARGE 				= 0x02,
	CSE_FAST_CHARGE 			= 0x03,
	CSE_TAPER_CHARGE 			= 0x04,
	CSE_TOP_OFF_TIMER_CHARGING 	= 0x05,
	CSE_CHARGE_TERMINATION_DONE = 0x06
}ChargeStatusEnum;

typedef struct
{
	uint8_t CHRG_STAT: 		3;
	uint8_t WD_STAT: 		1;
	uint8_t TREG_STAT: 		1;
	uint8_t VINDPM_STAT: 	1;
	uint8_t IINDPM_STAT: 	1;
	uint8_t RESERVED:	 	1;
}__attribute__((__packed__)) ChargerStatus1RegisterMask;

typedef enum
{
	VDSE_NO_INPUT 				= 0x00,
	VDSE_USB_HOST_SDP	 		= 0x01,
	VDSE_USB_CDP	 			= 0x02,
	VDSE_ADAPTER				= 0x03,
	VDSE_POORSRC				= 0x04,
	VDSE_UNKNOWN_ADAPTER		= 0x05,
	VDSE_NON_STANDARD_ADAPTER	= 0x06,
	VDSE_OTG					= 0x07
}VBUSDetectionStatusEnum;

typedef struct
{
	uint8_t VSYS_STAT: 		1;
	uint8_t ICO_STAT: 		2;
	uint8_t RESERVED: 		1;
	uint8_t VBUS_STAT: 		3;
	uint8_t PG_STAT:	 	1;
}__attribute__((__packed__)) ChargerStatus2RegisterMask;


typedef enum
{
	NTCSE_NORMAL			= 0x00,
	NTCSE_TS_WARM 			= 0x02,
	NTCSE_TS_COOL 			= 0x03,
	NTCSE_TS_COLD 			= 0x05,
	NTCSE_TS_HOT			= 0x06
}NTCStatusEnum;

typedef struct
{
	uint8_t TS_STAT: 		3;
	uint8_t RESERVED:	 	5;
}__attribute__((__packed__)) NTCStatusRegisterMask;

typedef struct
{
	uint8_t RESERVED:	 		4;
	uint8_t TMR_STAT: 			1;
	uint8_t BATOVP_STAT:	 	1;
	uint8_t TSHUT_STAT: 		1;
	uint8_t VBUS_OVP_STAT:	 	1;
}__attribute__((__packed__)) FAULTStatusRegisterMask;

typedef struct
{
	ChargerStatus1RegisterMask  Status1;
	ChargerStatus2RegisterMask  Status2;
	NTCStatusRegisterMask 		NTC_Status;
	FAULTStatusRegisterMask		Fault_Status;


}__attribute__((__packed__)) ChargerStatusMask;


typedef struct
{
	uint8_t RESERVED: 		4;
	uint8_t TMR_STAT: 		1;
	uint8_t BATOVP_STAT: 	1;
	uint8_t TSHUT_STAT: 	1;
	uint8_t VBUS_OVP_STAT: 	1;
}__attribute__((__packed__)) FAULT_StatusRegisterMask;

typedef struct
{
	uint8_t ICHG:		6;
	uint8_t EN_ILIM: 	1;
	uint8_t EN_HIZ: 	1;
	uint16_t ChargeCurrent_mA;
}__attribute__((__packed__)) ChargerCurrentLimitRegisterMask;

typedef struct
{
	uint8_t ICO_ILIM:	4;
	uint8_t RESERVED: 	4;
	uint16_t CurrentLimit_mA;
}__attribute__((__packed__)) ICO_CurrentLimitRegisterMask;


void BQ25883_init(BQ25883_init_struct* init_struct);

BQ25883_I2C_STATUS_ENUM BQ25883_GetRegister(uint8_t reg_num, uint8_t* value);
BQ25883_I2C_STATUS_ENUM BQ25883_SetRegister(uint8_t reg_num, uint8_t* value);

BQ25883_I2C_STATUS_ENUM BQ25883_Reset();

BQ25883_I2C_STATUS_ENUM BQ25883_GetPartInfo(uint8_t* value);

BQ25883_I2C_STATUS_ENUM BQ25883_SetMaxBatteryChargeVoltage(uint16_t value_mV);
BQ25883_I2C_STATUS_ENUM BQ25883_GetMaxBatteryChargeVoltage(uint16_t* value);

BQ25883_I2C_STATUS_ENUM BQ25883_SetBatteryChargeCurrent(bool updateFullRegistr, ChargerCurrentLimitRegisterMask RegField, uint16_t value_mA);
BQ25883_I2C_STATUS_ENUM BQ25883_GetBatteryChargeCurrent(ChargerCurrentLimitRegisterMask* value);

BQ25883_I2C_STATUS_ENUM BQ25883_SetMinInputVoltage(bool updateFullRegistr, InputVoltageLimitRegisterMask RegField, uint16_t value_mV);
BQ25883_I2C_STATUS_ENUM BQ25883_GetMinInputVoltage(InputVoltageLimitRegisterMask* value);

BQ25883_I2C_STATUS_ENUM BQ25883_SetMinInputCurrent(bool updateFullRegistr, InputCurrentLimitRegisterMask RegField, uint16_t value_mA);

BQ25883_I2C_STATUS_ENUM BQ25883_Set2ControlRegister(ChargerControl2RegisterMask RegField);
BQ25883_I2C_STATUS_ENUM BQ25883_Get2ControlRegister(ChargerControl2RegisterMask* RegField);

BQ25883_I2C_STATUS_ENUM BQ25883_SetADCControlRegister(ADCControlRegisterMask RegField);
BQ25883_I2C_STATUS_ENUM BQ25883_GetADCControlRegister(ADCControlRegisterMask* value);

BQ25883_I2C_STATUS_ENUM BQ25883_SetADCFunctionDisableRegister(ADCFunctionDisableRegisterMask RegField);

BQ25883_I2C_STATUS_ENUM BQ25883_GetChargeStatus1Register(ChargerStatus1RegisterMask* value);

BQ25883_I2C_STATUS_ENUM BQ25883_GetChargerStatus(ChargerStatusMask* status);
BQ25883_I2C_STATUS_ENUM BQ25883_GetChargerCurrentLimitICO(ICO_CurrentLimitRegisterMask* value);


BQ25883_I2C_STATUS_ENUM BQ25883_GetIBUS(int16_t* value);
BQ25883_I2C_STATUS_ENUM BQ25883_GetICHG(int16_t* value);
BQ25883_I2C_STATUS_ENUM BQ25883_GetVBUS(int16_t* value);
BQ25883_I2C_STATUS_ENUM BQ25883_GetVBAT(int16_t* value);
BQ25883_I2C_STATUS_ENUM BQ25883_GetVSYS(int16_t* value);
BQ25883_I2C_STATUS_ENUM BQ25883_GetTS(int16_t* value);
BQ25883_I2C_STATUS_ENUM BQ25883_GetTDIE(int16_t* value);

BQ25883_I2C_STATUS_ENUM BQ25883_GetADC(uint8_t Reg, int16_t* value);

#endif /* ENABLE_BQ25883 */

#endif /* SRC_BQ25883_H_ */
