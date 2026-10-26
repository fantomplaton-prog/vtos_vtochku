/*
  ******************************************************************************
  * @file    BQ25883.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "BQ25883.h"


#ifdef ENABLE_BQ25883

BQ25883_init_struct* config;


void BQ25883_init(BQ25883_init_struct* init_struct)
{
	config = init_struct;
}


BQ25883_I2C_STATUS_ENUM BQ25883_GetRegister(uint8_t reg_num, uint8_t* value)
{
  return config->read_func(BQ25883_WRITE_ADDRESS,reg_num,(uint8_t*)value,1);
}
BQ25883_I2C_STATUS_ENUM BQ25883_SetRegister(uint8_t reg_num, uint8_t* value)
{
  return config->write_func(BQ25883_WRITE_ADDRESS,reg_num,(uint8_t*)value,1);
}


BQ25883_I2C_STATUS_ENUM BQ25883_Reset()
{
	uint8_t value = 0x80;
	return config->write_func(BQ25883_READ_ADDRESS,BQ2588X_REG_PART_NUM,&value,1);
}

BQ25883_I2C_STATUS_ENUM  BQ25883_GetPartInfo(uint8_t* value)
{
	return config->read_func(BQ25883_READ_ADDRESS,BQ2588X_REG_PART_NUM,value,1);
}


BQ25883_I2C_STATUS_ENUM BQ25883_SetMinInputVoltage(bool updateFullRegistr, InputVoltageLimitRegisterMask RegField, uint16_t value_mV)
{
	uint8_t value = 0;

	InputVoltageLimitRegisterMask* reg_value = (InputVoltageLimitRegisterMask*)&value;

	if(updateFullRegistr)
	{
		if(value_mV > 3900)
		{
			value_mV -= 3900;
			value_mV /= 100;

			reg_value->EN_BAT_DISCHG = RegField.EN_BAT_DISCHG;
			reg_value->EN_VINDPM_RST = RegField.EN_VINDPM_RST;
			reg_value->PFM_OOA_DIS = RegField.PFM_OOA_DIS;
			reg_value->VINDPM = value_mV & 0x1F;

			return config->write_func(BQ25883_WRITE_ADDRESS,BQ2588X_REG_VINDPM,&value,1);
		}else
		{
			reg_value->EN_BAT_DISCHG = RegField.EN_BAT_DISCHG;
			reg_value->EN_VINDPM_RST = RegField.EN_VINDPM_RST;
			reg_value->PFM_OOA_DIS = RegField.PFM_OOA_DIS;
			reg_value->VINDPM = 0;

			return config->write_func(BQ25883_WRITE_ADDRESS,BQ2588X_REG_VINDPM,&value,1);
		}
	}
}

BQ25883_I2C_STATUS_ENUM BQ25883_GetMinInputVoltage(InputVoltageLimitRegisterMask* value)
{

	BQ25883_I2C_STATUS_ENUM status = config->read_func(BQ25883_READ_ADDRESS,BQ2588X_REG_VINDPM,value,1);
	if(status == BQ25883_I2C_STATUS_OK)
	{
	//value->EN_BAT_DISCHG = (*((InputVoltageLimitRegisterMask*)value)).EN_BAT_DISCHG;
	//str.EN_VINDPM_RST = (*((InputVoltageLimitRegisterMask*)value)).EN_VINDPM_RST;
	//str.PFM_OOA_DIS = (*((InputVoltageLimitRegisterMask*)value)).PFM_OOA_DIS;
	//str.VINDPM = (*((InputVoltageLimitRegisterMask*)value)).VINDPM;
	value->MinVoltage_mV = ((uint16_t)value->VINDPM*100)+3900;
	}

	return status;
}

BQ25883_I2C_STATUS_ENUM BQ25883_SetMinInputCurrent(bool updateFullRegistr, InputCurrentLimitRegisterMask RegField, uint16_t value_mA)
{
	uint8_t value = 0;

	InputCurrentLimitRegisterMask* reg_value = (InputCurrentLimitRegisterMask*)&value;

	if(updateFullRegistr)
	{
		if(value_mA > 500)
		{
			value_mA -= 500;
			value_mA /= 100;

			reg_value->EN_ICO = RegField.EN_ICO;
			reg_value->FORCE_ICO = RegField.FORCE_ICO;
			reg_value->FORCE_INDET = RegField.FORCE_INDET;
			reg_value->IINDPM = value_mA & 0x1F;

			return config->write_func(BQ25883_WRITE_ADDRESS,BQ2588X_REG_IINDPM,&value,1);
		}else
		{
			reg_value->EN_ICO = RegField.EN_ICO;
			reg_value->FORCE_ICO = RegField.FORCE_ICO;
			reg_value->FORCE_INDET = RegField.FORCE_INDET;
			reg_value->IINDPM = 0;

			return config->write_func(BQ25883_WRITE_ADDRESS,BQ2588X_REG_IINDPM,&value,1);
		}
	}
}

BQ25883_I2C_STATUS_ENUM BQ25883_SetMaxBatteryChargeVoltage(uint16_t value_mV)
{
	uint8_t value = 0;

	if(value_mV > 6800)
	{
		value_mV -= 6800;
		value_mV /= 10;

		value = (uint8_t)value_mV;

		return config->write_func(BQ25883_WRITE_ADDRESS,BQ2588X_REG_CHARGE_VOLT,&value,1);

	}else
	{
		return config->write_func(BQ25883_WRITE_ADDRESS,BQ2588X_REG_CHARGE_VOLT,&value,1);
	}
}

BQ25883_I2C_STATUS_ENUM BQ25883_GetMaxBatteryChargeVoltage(uint16_t* value)
{
	uint8_t value8 = 0;

	BQ25883_I2C_STATUS_ENUM status = config->read_func(BQ25883_READ_ADDRESS,BQ2588X_REG_CHARGE_VOLT,&value8,1);
	if(status == BQ25883_I2C_STATUS_OK)
	{
		*value = ((uint16_t)value8*10)+6800;
	}

	return status;
}


BQ25883_I2C_STATUS_ENUM BQ25883_GetChargerCurrentLimitICO(ICO_CurrentLimitRegisterMask* value)
{

	BQ25883_I2C_STATUS_ENUM status = config->read_func(BQ25883_READ_ADDRESS,BQ2588X_REG_CHARGE_VOLT,(uint8_t*)value,1);
	if(status == BQ25883_I2C_STATUS_OK)
	{
		value->CurrentLimit_mA = (((uint16_t)value->ICO_ILIM)*100)+500;
	}

	return status;
}

BQ25883_I2C_STATUS_ENUM BQ25883_SetBatteryChargeCurrent(bool updateFullRegistr, ChargerCurrentLimitRegisterMask RegField, uint16_t value_mA)
{
	uint8_t value = 0;

	ChargerCurrentLimitRegisterMask* reg_value = (ChargerCurrentLimitRegisterMask*)&value;

	if(updateFullRegistr)
	{
		if(value_mA > 100)
		{
			value_mA -= 100;
			value_mA /= 50;

			reg_value->EN_HIZ = RegField.EN_HIZ;
			reg_value->EN_ILIM = RegField.EN_ILIM;
			reg_value->ICHG = value_mA & 0x3F;

			return config->write_func(BQ25883_WRITE_ADDRESS,BQ2588X_REG_CHARGE_CURRENT,&value,1);
		}else
		{
			reg_value->EN_HIZ = RegField.EN_HIZ;
			reg_value->EN_ILIM = RegField.EN_ILIM;
			reg_value->ICHG = 0;

			return config->write_func(BQ25883_WRITE_ADDRESS,BQ2588X_REG_CHARGE_CURRENT,&value,1);
		}
	}
}

BQ25883_I2C_STATUS_ENUM BQ25883_GetBatteryChargeCurrent(ChargerCurrentLimitRegisterMask* value)
{
	BQ25883_I2C_STATUS_ENUM status = config->read_func(BQ25883_READ_ADDRESS,BQ2588X_REG_CHARGE_CURRENT,(uint8_t*)value,1);
	if(status == BQ25883_I2C_STATUS_OK)
	{
		value->ChargeCurrent_mA = ((uint16_t)value->ICHG*50)+100;
	}
	return status;

}

BQ25883_I2C_STATUS_ENUM BQ25883_Set2ControlRegister(ChargerControl2RegisterMask RegField)
{
	return config->write_func(BQ25883_WRITE_ADDRESS,BQ2588X_REG_CHG_CTRL2,(uint8_t*)&RegField,1);
}
BQ25883_I2C_STATUS_ENUM BQ25883_Get2ControlRegister(ChargerControl2RegisterMask* RegField)
{
	return config->read_func(BQ25883_READ_ADDRESS,BQ2588X_REG_CHG_CTRL2,(uint8_t*)RegField,1);
}

BQ25883_I2C_STATUS_ENUM  BQ25883_SetADCControlRegister(ADCControlRegisterMask RegField)
{
	return config->write_func(BQ25883_WRITE_ADDRESS,BQ2588X_REG_ADC_CTRL,(uint8_t*)&RegField,1);
}

BQ25883_I2C_STATUS_ENUM BQ25883_GetADCControlRegister(ADCControlRegisterMask* value)
{
	return config->read_func(BQ25883_READ_ADDRESS,BQ2588X_REG_ADC_CTRL,(uint8_t*)value,1);
}

BQ25883_I2C_STATUS_ENUM BQ25883_SetADCFunctionDisableRegister(ADCFunctionDisableRegisterMask RegField)
{
	return config->write_func(BQ25883_WRITE_ADDRESS,BQ2588X_REG_ADC_FUNC_DIS,(uint8_t*)&RegField,1);
}

BQ25883_I2C_STATUS_ENUM BQ25883_GetChargeStatus1Register(ChargerStatus1RegisterMask* value)
{
  return config->read_func(BQ25883_READ_ADDRESS,BQ2588X_REG_CHG_STATUS1,(uint8_t*)value,1);
}

BQ25883_I2C_STATUS_ENUM BQ25883_GetChargerStatus(ChargerStatusMask* status)
{
	return config->read_func(BQ25883_READ_ADDRESS,BQ2588X_REG_CHG_STATUS1,(uint8_t*)status,4);
}

BQ25883_I2C_STATUS_ENUM BQ25883_GetADC(uint8_t Reg, int16_t* value)
{
	BQ25883_I2C_STATUS_ENUM status;
	uint8_t t_value[2];

	status = config->read_func(BQ25883_READ_ADDRESS,Reg,t_value,2);
	if(status == BQ25883_I2C_STATUS_OK)
	{
		*value = 0;
		*value |= t_value[0]<<8;
		*value |= t_value[1];
	}

	return status;
}


BQ25883_I2C_STATUS_ENUM  BQ25883_GetVBAT(int16_t* out_value)
{
	return BQ25883_GetADC(BQ2588X_REG_VBAT_ADC_1,out_value);
}

BQ25883_I2C_STATUS_ENUM  BQ25883_GetVBUS(int16_t* out_value)
{
	return BQ25883_GetADC(BQ2588X_REG_VBUS_ADC_1,out_value);
}

BQ25883_I2C_STATUS_ENUM BQ25883_GetIBUS(int16_t* value)
{
	return BQ25883_GetADC(BQ2588X_REG_IBUS_ADC_1,value);
}

BQ25883_I2C_STATUS_ENUM BQ25883_GetICHG(int16_t* value)
{
	return BQ25883_GetADC(BQ2588X_REG_ICHG_ADC_1,value);
}

BQ25883_I2C_STATUS_ENUM BQ25883_GetVSYS(int16_t* value)
{
	return BQ25883_GetADC(BQ2588X_REG_VSYS_ADC_1,value);
}

BQ25883_I2C_STATUS_ENUM BQ25883_GetTS(int16_t* value)
{
	return BQ25883_GetADC(BQ2588X_REG_TS_ADC_1,value);
}
BQ25883_I2C_STATUS_ENUM BQ25883_GetTDIE(int16_t* value)
{
	return BQ25883_GetADC(BQ2588X_REG_TDIE_ADC_1,value);
}

#endif /* ENABLE_BQ25883 */
