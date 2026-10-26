/*
  ******************************************************************************
  * @file    BQ25883_Controller.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "BQ25883_Controller.h"
#include "BQ25883.h"

#ifdef ENABLE_BQ25883





BQ25883_init_struct init_struct;

void BQ25883_Init_Controller()
{

	init_struct.read_func = BQ25883_Read;
	init_struct.write_func = BQ25883_Write;
	BQ25883_init(&init_struct);

	uint8_t PartInfo = 0;

	BQ25883_Reset();
	BQ25883_Dalay(5);

	BQ25883_GetPartInfo(&PartInfo);

	BQ25883_SetMaxBatteryChargeVoltage(8400);
	BQ25883_Dalay(5);

	ChargerCurrentLimitRegisterMask CCLRM;
	CCLRM.EN_HIZ = 0;
	CCLRM.EN_ILIM = 1;
	CCLRM.ICHG = 0;
	BQ25883_SetBatteryChargeCurrent(true,CCLRM,1500);
	BQ25883_Dalay(5);

	InputVoltageLimitRegisterMask IVLRM;
	IVLRM.EN_BAT_DISCHG = 0;
	IVLRM.EN_VINDPM_RST = 1;
	IVLRM.PFM_OOA_DIS = 0;
	IVLRM.VINDPM = 0;
	BQ25883_SetMinInputVoltage(true, IVLRM, 4500);
	BQ25883_Dalay(5);

	InputCurrentLimitRegisterMask IILRM;
	IILRM.EN_ICO = 1;
	IILRM.FORCE_ICO = 0;
	IILRM.FORCE_INDET = 0;
	IILRM.IINDPM = 0;
	BQ25883_SetMinInputCurrent(true,  IILRM, 2000);
	BQ25883_Dalay(5);

	ChargerControl2RegisterMask CC2RM;
	CC2RM.AUTO_INDET_EN = 0;
	CC2RM.EN_OTG = 0;
	CC2RM.TREG = TREG_120C;
	CC2RM.EN_CHG = 1;
	CC2RM.BATLOWV = 1;
	CC2RM.VRECHG = VRECHG_200_MV;
	BQ25883_Set2ControlRegister(CC2RM);
	BQ25883_Dalay(5);


	ADCFunctionDisableRegisterMask ADCFDRM;
	ADCFDRM.IBUS_ADC_DIS = 0;
	ADCFDRM.ICHG_ADC_DIS = 0;
	ADCFDRM.RESERVED = 0;
	ADCFDRM.TDIE_ADC_DIS = 0;
	ADCFDRM.TS_ADC_DIS = 0;
	ADCFDRM.VBAT_ADC_DIS = 0;
	ADCFDRM.VBUS_ADC_DIS = 0;
	ADCFDRM.VSYS_ADC_DIS = 0;
	BQ25883_SetADCFunctionDisableRegister(ADCFDRM);
	BQ25883_Dalay(5);

	ADCControlRegisterMask ADCCRM;
	ADCCRM.ADC_EN = 1;
	ADCCRM.ADC_RATE = CONTINUOUS_CONVERSION;
	ADCCRM.ADC_SAMPLE = RESOLUTION_15_BIT;
	BQ25883_SetADCControlRegister(ADCCRM);
	BQ25883_Dalay(5);
}

#endif // ENABLE_BQ25883
