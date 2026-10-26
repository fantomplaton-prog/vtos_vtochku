/*
  ******************************************************************************
  * @file    device_info.h
  * @author  A.Alborov
  ******************************************************************************
*/

/*

#ifdef example

#ifndef INC_DEVICE_INFO_H_
#define INC_DEVICE_INFO_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "Data/data_table.h"
#include "Data/device_info_structs.h"
#include "Data/base_device_info.h"
#include "Devices/base_virtual_device.h"
#include "Data/base_parameter.h"



 typedef enum {
 		DSE_BOOTLOADER_INFO_TYPE,
 		DSE_FW_INFO_TYPE,
 		DSE_DEVICE_INFO_TYPE,
 		DSE_BOARD_INFO_TYPE,
 		DSE_PRODUCT_INFO_TYPE,
 		DSE_SERVICE_INFO,
 		DSE_SYSTEM_DEVICES,
		DSE_REO_SETTINGS,
 		DSE_NUM
 }device_slots_enum ;




 typedef enum {
	PCE_REO_SETTINGS = 0,
	PCE_REO_SIGNALS_0_5,
	PCE_REO_SIGNALS_6_11,
	PCE_REO_SIGNALS_12_17,
	PCE_REO_SIGNALS_18_23,
	PCE_NUM
 }this_device_parametrs_code_enum;


 typedef struct{
 	uint8_t REO_ch_settings_byte_0;
	uint8_t REO_ch_settings_byte_N;
 }__attribute__((__packed__))ROM_REO_ch_settings_struct;


 typedef struct{
 	uint8_t REO_settings_byte_0;
	uint8_t REO_settings_byte_N;

	ROM_REO_ch_settings_struct REO_settings[N];

 }__attribute__((__packed__))ROM_REO_settings_struct;


  typedef struct{
 	uint8_t REO_settings_byte_0;
	uint8_t REO_settings_byte_N;

 }__attribute__((__packed__))USER_REO_settings_struct;




extern base_parameter_info_struct this_device_parametrs[PCE_NUM];

extern base_device_information_struct* device_info;

void init_device_info();





#ifdef __cplusplus
}
#endif

#endif /* INC_DEVICE_INFO_H_ */



#endif

