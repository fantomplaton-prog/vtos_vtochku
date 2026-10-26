/*
  ******************************************************************************
  * @file    device_command_structs.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef INC_DEVICE_COMMAND_STRUCTS_H_
#define INC_DEVICE_COMMAND_STRUCTS_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "UBI_SDK.h"


 typedef struct
 {
 	char DeviceName[32];
 	uint8_t DeviceVersion;
 	uint8_t DeviceSubVersion;
 	char DeviceLabel[16];
 	uint8_t DeviceType;
 	uint8_t DeviceCode;

 	uint8_t DeviceAddress;

 	char DeviceUUID_Prefix[16];
 	uint32_t DeviceUUID;

 	uint8_t ParametersNum;
 	uint8_t SubDeviceNum;
 	uint8_t ProgramType;
 	uint8_t DefaultPort;

 }__attribute__((__packed__))full_inicialize_connection_struct;


 typedef struct
 {
 	uint8_t DeviceCode;
 	uint8_t DeviceRole;
 	uint8_t DeviceVersion;
 	uint8_t DeviceSubVersion;

 	uint8_t DeviceType;

 	uint8_t DeviceAddress;

 	uint8_t ParametersNum;

 	uint8_t SubDeviceNum;

 	uint8_t ProgramType;

 	uint8_t DefaultPort;

 }__attribute__((__packed__))short_inicialize_connection_struct;



#ifdef __cplusplus
}
#endif

#endif /* INC_DEVICE_COMMAND_STRUCTS_H_*/
