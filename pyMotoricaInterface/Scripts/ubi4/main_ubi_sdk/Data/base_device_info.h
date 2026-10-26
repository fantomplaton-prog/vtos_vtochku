/*
  ******************************************************************************
  * @file    base_device_info.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef INC_BASE_DEVICE_INFO_H_
#define INC_BASE_DEVICE_INFO_H_

#include "stdint.h"
#include "stdbool.h"
#include "Data/data_table.h"
#include "Data/device_info_structs.h"
#include "Data/base_parameter.h"
#include "Devices/enum_discription.h"
#include "Drivers/base_memory_driver.h"
#include "TransportLevel/a_protocol_struct.h"







typedef struct
{
	base_memory_driver_struct* memory_driver;
	data_table_struct* data_table;

	uint32_t DataTableStartAddres;

	data_table_default_struct default_data_table;

	base_parameter_info_struct* param_info_struct;
	base_parameter_pointers_struct* param_pointer_struct;
	uint8_t param_num;

	void (*CustomDataManagerCommand)(void* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
	void (*CustomDeviceInformationCommand)(void* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
	void (*ReadDeviceInformationCommandCallBack)(void* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);

}init_device_information_struct;

typedef struct
{
	init_device_information_struct* init_device_info;
	baseDeviceInfo_struct* DeviceInfo;


}base_device_information_struct;






void* getDefaultDataTables();

void init_base_device_info(init_device_information_struct* init_device_info,base_device_information_struct* device_info);


void ParameterRequestHandler(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
void InicializeConnection_v2(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
void DataManagerCommand_v2(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
void ResetToFactoryCommand_v2(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
void GetSerialNumber_v2(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
void DeviceInformationCommand_v2(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
void Data_Transfer_Settings(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);

void Sub_Device_Manager(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);

void FullResetToFactory(base_device_information_struct* deviceInfo);

void Reinit_DeviceInfo_Callback(base_device_information_struct* deviceInfo);

void base_device_CustomDataManagerCommand(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
void base_device_CustomDeviceInformationCommand(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
void Complex_Parameter_Transfer(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
void Data_Transfer_Setting(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);

//void FullResetToFactory();



#endif /* INC_BASE_DEVICE_INFO_H_ */
