/*
 * master_device.h
 *
 *  Created on: 15.07.24
 *      Author: PC
 */

#ifndef DEVICES_MASTER_DEVICE_H_
#define DEVICES_MASTER_DEVICE_H_

#include "Data/base_device_info.h"
#include "UBI_SDK.h"

//#define UBI_SDK_SYSTEM_ROLE_MASTER

#ifdef UBI_SDK_SYSTEM_ROLE_MASTER

#include "device_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "string.h"


typedef struct
{
	device_parameters_struct* Device;
	base_parameter_struct* Parameter;
	uint8_t exist_change;
}stream_parameter_struct;

typedef struct
{
	stream_parameter_struct* stream_param;
	uint16_t Count;
}stream_parameters_struct;

typedef struct
{
	base_device_information_struct Base;
	device_manager_struct* device_manager;

	external_device_parameter_stream__struct* external_device_stream;
	stream_parameters_struct* stream_structs;

}master_device_struct;







void init_master_device(init_device_information_struct* init_device_info,master_device_struct* device_info);

void WriteParameterInThisDevice(base_parameter_struct* param, uint8_t* Data);
void WriteParameterInSubDevice(device_parameters_struct* device, base_parameter_struct* param, uint8_t* Data);
void UpdateStreamingParameter(base_parameter_struct* param, uint8_t* Data);

void master_WriteParameter(base_parameter_struct* Param, uint8_t* Data);
void UpdateDependence(base_parameter_struct* param);

void ReadSubDeviceParameters(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str);
void ReadSubDeviceAdditionalParameters(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str);
void SubDeviceParameterInitRead(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str);
void SubDeviceParameterInitWrite(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str);

void StopParameterTransfer(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
void localStopParameterTransfer();

#endif /* UBI_SDK_SYSTEM_ROLE_MASTER */

#endif /* DEVICES_MASTER_DEVICE_H_ */
