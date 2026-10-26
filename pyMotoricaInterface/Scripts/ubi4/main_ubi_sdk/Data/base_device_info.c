/*
  ******************************************************************************
  * @file    base_device_info.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "Data/base_device_info.h"
#include "Drivers/esp32/flash_driver.h"
#include "Drivers/esp32/partition_flash_driver.h"
#include "Data/data_table.h"
#include "Data/device_command_structs.h"

#include "string.h"
#include "stdlib.h"
#include "UBI_SDK.h"
#include "stdlib.h"



#ifdef UBI_SDK_SYSTEM_ROLE_MASTER
#include "request_scheduler.h"
#endif



void InicializeConnection(command_selector_struct command);
void ClearDataTable(data_table_struct* data_table);




void Save_Data(data_table_struct* data_table, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);

void Set_Brige_Mode(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);



__attribute__((weak))
void base_device_CustomDataManagerCommand(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	if(deviceInfo->init_device_info->CustomDataManagerCommand)
	{
		deviceInfo->init_device_info->CustomDataManagerCommand(deviceInfo,protocol_str,receive_str);
	}
}


void DataManagerCommand_v2(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	data_table_struct* t_data_table = deviceInfo->init_device_info->data_table;
	switch(receive_str.data_buf[0])
	{
		case READ_AVAILABLE_SLOTS:		ReadAvailableSlot_v2(t_data_table, protocol_str,receive_str);			break;
		case READ_DATA:					ReadDataInSlot_v2(t_data_table, protocol_str,receive_str);				break;
		case WRITE_DATA:				WriteDataInSlot_v2(t_data_table, protocol_str,receive_str); 			break;
		case RESET_TO_FACTORY:			ResetToFactoryCommand_v2(deviceInfo,protocol_str,receive_str); 			break;
		case SAVE_DATA: 				Save_Data(t_data_table,protocol_str,receive_str);						break;
		case READ_SLOT:					ReadSlot(t_data_table, protocol_str,receive_str);						break;

		default:
		{
			base_device_CustomDataManagerCommand(deviceInfo,protocol_str,receive_str);
			break;
		}
	}

}

void ResetToFactoryCommand_v2(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	uint8_t slot = receive_str.data_buf[1];

	data_table_write_data_status_enum status;

	if(receive_str.data_buf[1] == 0xFF)
	{
		FullResetToFactory(deviceInfo);
	}else
	{
		data_table_struct* data_table = deviceInfo->init_device_info->data_table;

	//	DataTableWriteData(data_table, slot, &receive_str.data_buf[1], &status);
	}
}

void Save_Data(data_table_struct* data_table, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	DataTable_Save(data_table);
	//DataTable_Close(data_table);

//	DataTable_Clear_RAM(data_table);
//	DataTable_Open(data_table);

}


void GetSerialNumber_v2(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{


	char* SerialString = "SubModule CPU 12345";


	base_protocol_data_segment_struct* data_segment = protocol_str->get_data_segment_func(protocol_str, strlen(SerialString));

	if(!data_segment) return;

	char* Serial = (char*)data_segment->pData;

//	memcpu(Serial,SerialString,strlen(SerialString));

	protocol_str->protocol_answer_obj_data_func(protocol_str,receive_str.pack_info, data_segment);
}

void ReadDeviceAdditionalParameterSize(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{

	uint8_t paramID = receive_str.data_buf[1];

	if(deviceInfo->init_device_info->param_num <= paramID) return;

	uint8_t addInfoNum = deviceInfo->init_device_info->param_info_struct[paramID].additional_info_size;

	if(addInfoNum == 0)return;

	uint16_t allsize = addInfoNum*sizeof(base_parameter_addinfoseg_struct);

	for(uint8_t i = 0; i<addInfoNum; i++)
				allsize += deviceInfo->init_device_info->param_pointer_struct[paramID].add_info_segments[i].info_size;

	base_protocol_data_segment_struct* data_segment = protocol->get_data_segment_func(protocol, 2+sizeof(uint16_t));

	if(!data_segment) return;

	data_segment->pData[0] = READ_DEVICE_ADDITIONAL_PARAMETER_SIZE;
	data_segment->pData[1] = paramID;
	*((uint16_t*)&data_segment->pData[2]) = allsize;

	if(protocol->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
				protocol->protocol_answer_obj_data_func(protocol, receive_str.pack_info, data_segment);

}


void ReadDeviceAdditionalParameters(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{

	uint8_t paramID = receive_str.data_buf[1];
	
	if(deviceInfo->init_device_info->param_num <= paramID) return;

	uint8_t addInfoNum = deviceInfo->init_device_info->param_info_struct[paramID].additional_info_size;

	if(addInfoNum == 0)return;

	uint16_t allsize = 2 + (addInfoNum*sizeof(base_parameter_addinfoseg_struct));
	
	uint16_t h_size = allsize;

	for(uint8_t i = 0; i<addInfoNum; i++)
				allsize += deviceInfo->init_device_info->param_pointer_struct[paramID].add_info_segments[i].info_size;
	

	base_protocol_data_segment_struct* data_segment = protocol->get_data_segment_func(protocol, allsize);

	if(!data_segment) return;

	uint8_t* seg_p = (uint8_t*)deviceInfo->init_device_info->param_pointer_struct[paramID].add_info_segments;

	data_segment->pData[0] = READ_DEVICE_ADDITIONAL_PARAMETER;
	data_segment->pData[1] = paramID;
	memcpy(data_segment->pData + 2, seg_p, h_size - 2);
	
	uint16_t data_counter = 0;
	for(uint8_t i = 0; i<addInfoNum; i++)
	{
		uint8_t* info_p = deviceInfo->init_device_info->param_pointer_struct[paramID].add_info_segments[i].info;
		uint16_t info_size = deviceInfo->init_device_info->param_pointer_struct[paramID].add_info_segments[i].info_size;
		
		memcpy(data_segment->pData + h_size + data_counter, info_p, info_size);
		
		data_counter+=info_size;
	}


	if(protocol->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
				protocol->protocol_answer_obj_data_func(protocol, receive_str.pack_info, data_segment);

}




void ReadDeviceParameters(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{

	if(deviceInfo->init_device_info->param_num > 0)
	{
		
		uint8_t param_start_index = receive_str.data_buf[1];
		uint8_t param_count = receive_str.data_buf[2];
		
		
		
	base_protocol_data_segment_struct* data_segment = protocol->get_data_segment_func(protocol, 3 + sizeof(base_parameter_info_struct)*param_count);

	if(!data_segment) return;

	data_segment->pData[0] = READ_DEVICE_PARAMETERS;
	data_segment->pData[1] = param_start_index;	
	data_segment->pData[2] = param_count;
		
	memcpy(data_segment->pData + 3, &deviceInfo->init_device_info->param_info_struct[param_start_index], sizeof(base_parameter_info_struct)*param_count);


	if(protocol->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
				protocol->protocol_answer_obj_data_func(protocol, receive_str.pack_info, data_segment);
	}
}



__attribute__((weak))
void ParameterRequestHandler(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{

	bool write = receive_str.command & 0x80;
	uint8_t paramPosition = receive_str.command & 0x7F;




	base_parameter_info_struct* device_parameters_i = deviceInfo->init_device_info->param_info_struct;
	base_parameter_pointers_struct* device_parameters_pointer =  deviceInfo->init_device_info->param_pointer_struct;

	uint8_t param_num = deviceInfo->init_device_info->param_num;


	if(paramPosition < param_num)
	{
		if(write)
		{
			if((device_parameters_i[paramPosition].synch_type == PSTE_WRITE_ONLY) ||
				(device_parameters_i[paramPosition].synch_type == PSTE_STREAMING) ||
				(device_parameters_i[paramPosition].synch_type == PSTE_READ_WRITE))
			{
			//	base_protocol_data_segment_struct* data_segment = protocol_str->get_data_segment_func(protocol_str,device_parameters[paramPosition].parameter_size);

				//if(!data_segment) return;

				//memcpy(data_segment->pData, device_parameters_pointer[paramPosition].parameter_data_p,device_parameters[paramPosition].parameter_size);

				//if(protocol_str->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
				//	protocol_str->protocol_answer_obj_data_func(protocol_str, receive_str.pack_info, data_segment);
				
				
				base_parameter_struct param;
				param.info = &device_parameters_i[paramPosition];
				param.pointers = &device_parameters_pointer[paramPosition];
				
				switch(device_parameters_i[paramPosition].type)
				{
					case PARTE_STRUCT_ARRAY_TYPE:
					{
						parameter_write_master_to_slave_struct_array( &param, receive_str.data_buf);
					}break;
					case PARTE_INT32_TYPE:
					{
						parameter_write_master_to_slave_simple( &param, receive_str.data_buf);
					}break;
					case PARTE_FIX_DATA_TRANSFER_TYPE:
					{
						parameter_write_master_to_slave_data_transfer( &param, receive_str.data_buf,receive_str.size);
						break;
					}
					
					default:
					{
						parameter_write_master_to_slave_simple( &param, receive_str.data_buf);
					}break;
					
				}
				


				
				protocol_str->remove_pack_info(protocol_str, receive_str.pack_info);
			}
		}else
		{
			//if((device_parameters_i[paramPosition].synch_type == PSTE_READ_ONLY) || (device_parameters_i[paramPosition].synch_type == PSTE_STREAMING))
			//{
			

			if(device_parameters_pointer[paramPosition].pre_read_callback)
			{
				base_parameter_struct param;
				param.info = &device_parameters_i[paramPosition];
				param.pointers = &device_parameters_pointer[paramPosition];

				if(device_parameters_i[paramPosition].type == PARTE_FIX_DATA_TRANSFER_TYPE)
				{
					((pre_read_data_transfer_callback_t)device_parameters_pointer[paramPosition].pre_read_callback)(&param, receive_str.data_buf, receive_str.size);
				}else
				{
					((pre_read_callback_t)device_parameters_pointer[paramPosition].pre_read_callback)(&param);
				}
			}


				base_protocol_data_segment_struct* data_segment = protocol_str->get_data_segment_func(protocol_str,device_parameters_i[paramPosition].parameter_size);

				if(!data_segment)
				    return;

				memcpy(data_segment->pData, device_parameters_pointer[paramPosition].parameter_data_p, device_parameters_i[paramPosition].parameter_size);
				data_segment->data_tx_callback = device_parameters_pointer[paramPosition].tx_notification;
				data_segment->object = &device_parameters_i[paramPosition];
				if(protocol_str->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
					protocol_str->protocol_answer_obj_data_func(protocol_str, receive_str.pack_info, data_segment);
			//}
		}
	}

}


/*
void Add_SubDevice(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{
	data_table_struct* data_table = deviceInfo->init_device_info->data_table;

	data_table_read_data_status_enum data_status = 0;
	baseSubDevicesArrayInfo_struct* SubDevicesArray = Data_Table_ReadData(data_table, DTCE_SYSTEM_DEVICES, &data_status);

	switch(data_status)
	{
		case DTRD_READ_DATA_OK:
		{
			break;
		}
		case DTRD_READ_RAM_IMAGE:
		{

			uint8_t count = SubDevicesArray->Size;
			baseSubDeviceInfo_struct* SubDevice = &SubDevicesArray->DevicesArray[count];
			memcpy(SubDevice, &receive_str.data_buf[1], sizeof(baseSubDeviceInfo_struct));
			SubDevicesArray->Size++;
			protocol->remove_pack_info(protocol, receive_str.pack_info);

		 	DataTable_Save(data_table);


			break;
		}
		case DTRD_SLOT_NOT_FOUND:
		{
			break;
		}
	}


}

void Remove_SubDevice(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{

}

void Get_All_SubDevice(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{

}
*/

__attribute__((weak))
void Sub_Device_Manager(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	/*
	switch(receive_str.data_buf[0])
	{
		case ADD_SUB_DEVICE: 			Add_SubDevice(deviceInfo, protocol_str,receive_str);  					break;
		case REMOVE_SUB_DEVICE: 		Remove_SubDevice(deviceInfo, protocol_str,receive_str); 				break;
		case GET_ALL_SUB_DEVICE:		Get_All_SubDevice(deviceInfo, protocol_str,receive_str); 				break;
	}
	*/
}


__attribute__((weak))
void base_device_CustomDeviceInformationCommand(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	if(deviceInfo->init_device_info->CustomDeviceInformationCommand)
	{
		deviceInfo->init_device_info->CustomDeviceInformationCommand(deviceInfo,protocol_str,receive_str);
	}
}

__attribute__((weak))
void Complex_Parameter_Transfer(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{

}

__attribute__((weak))
void Data_Transfer_Setting(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	switch(receive_str.data_buf[0])
	{
		case DTSE_PROTOCOL_PREAMBLE: protocol_str->parce_preamble(protocol_str, receive_str.data_buf+1, receive_str.size-1 ); break;
		default: break;
	}
}


void DeviceInformationCommand_v2(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{

	switch(receive_str.data_buf[0])
	{
		case INICIALIZE_INFORMATION:						InicializeConnection_v2(deviceInfo, protocol_str,receive_str);				break;
		case READ_DEVICE_PARAMETERS:						ReadDeviceParameters(deviceInfo, protocol_str,receive_str);					break;
		case READ_DEVICE_ADDITIONAL_PARAMETER:				ReadDeviceAdditionalParameters(deviceInfo, protocol_str,receive_str);		break;
		case READ_DEVICE_ADDITIONAL_PARAMETER_SIZE:			ReadDeviceAdditionalParameterSize(deviceInfo, protocol_str,receive_str);	break;

		case GET_SERIAL_NUMBER:								GetSerialNumber_v2(deviceInfo, protocol_str,receive_str);					break;
		case SET_SERIAL_NUMBER:													break;
		case GET_DEVICE_NAME:					break;

		case SET_DEVICE_NAME:					break;
		case GET_DEVICE_ROLE:			 		break;
		case SET_DEVICE_ROLE:			 		break;

		default:
		{
			base_device_CustomDeviceInformationCommand(deviceInfo, protocol_str,receive_str);
			break;
		}

	}
}

void Data_Transfer_Settings(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{

	switch(receive_str.data_buf[0])
	{
		//case SET_BRIGE_MODE:	Set_Brige_Mode(deviceInfo, protocol_str,receive_str);	break;
	}
}

void Set_Brige_Mode(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{


}


void InicializeConnection_v2(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	uint8_t* t_buf = 0;

	if(deviceInfo->init_device_info->ReadDeviceInformationCommandCallBack)
	{
		deviceInfo->init_device_info->ReadDeviceInformationCommandCallBack(deviceInfo, protocol_str, receive_str);
	}

	switch(receive_str.data_buf[1])
	{
	case 1 :
	{
		base_protocol_data_segment_struct* data_segment = protocol_str->get_data_segment_func(protocol_str, 2 + sizeof(short_inicialize_connection_struct));

		if(!data_segment) return;

		data_segment->pData[0] = INICIALIZE_INFORMATION;
		data_segment->pData[1] = 1;

		short_inicialize_connection_struct* inicialize_data = (short_inicialize_connection_struct*)&data_segment->pData[2];

		inicialize_data->DeviceAddress = deviceInfo->DeviceInfo->DeviceAddress;
		inicialize_data->DeviceCode = deviceInfo->DeviceInfo->DeviceCode;
		inicialize_data->DeviceType = deviceInfo->DeviceInfo->DeviceType;
		inicialize_data->DeviceRole = deviceInfo->DeviceInfo->DeviceRole;
		inicialize_data->DeviceVersion = deviceInfo->DeviceInfo->DeviceVersion;
		inicialize_data->DeviceSubVersion = deviceInfo->DeviceInfo->DeviceSubVersion;
		inicialize_data->ParametersNum = deviceInfo->init_device_info->param_num;
		inicialize_data->ProgramType = PTE_MAIN_PROGRAM;
		inicialize_data->SubDeviceNum = 0;

		if(protocol_str->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
					protocol_str->protocol_answer_obj_data_func(protocol_str, receive_str.pack_info, data_segment);
	}break;
	case 2 :
	{

		base_protocol_data_segment_struct* data_segment = protocol_str->get_data_segment_func(protocol_str, 2+sizeof(full_inicialize_connection_struct));

		if(!data_segment) return;


		data_table_read_data_status_enum status = 0;
		data_table_struct* data_table = deviceInfo->init_device_info->data_table;

		baseDeviceInfo_struct* DeviceInfo = Data_Table_ReadData(data_table,DTCE_DEVICE_INFO_TYPE,&status);
		if(status == DTRD_READ_RAM_IMAGE)
		{
			full_inicialize_connection_struct* inicialize_data = (full_inicialize_connection_struct*)(data_segment->pData + 2);


			memcpy(inicialize_data->DeviceName,DeviceInfo->DeviceName, 32);
			memcpy(inicialize_data->DeviceLabel,DeviceInfo->DeviceLabel, 16);
			memcpy(inicialize_data->DeviceUUID_Prefix,DeviceInfo->DeviceUUID_Prefix, 16);

			inicialize_data->DeviceCode = DeviceInfo->DeviceCode;
			inicialize_data->DeviceType = DeviceInfo->DeviceType;
			inicialize_data->DeviceVersion = DeviceInfo->DeviceVersion;
			inicialize_data->DeviceSubVersion = DeviceInfo->DeviceSubVersion;
			inicialize_data->DeviceUUID = DeviceInfo->DeviceUUID;
			inicialize_data->DeviceAddress = DeviceInfo->DeviceAddress;
			inicialize_data->ParametersNum = deviceInfo->init_device_info->param_num;
			inicialize_data->SubDeviceNum = 2;   // [PROBLEM] HARD CODE
			inicialize_data->ProgramType = PTE_BOOTLOADER;


			data_segment->pData[0] = INICIALIZE_INFORMATION;
			data_segment->pData[1] = 2;

			if(protocol_str->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
											protocol_str->protocol_answer_obj_data_func(protocol_str, receive_str.pack_info, data_segment);
		}


	}break;
	}

}



void* getDefaultDataTables()
{
	return 0;
}


void WriteDefaultInfo()
{

}

__attribute__((weak))
void Reinit_DeviceInfo_Callback(base_device_information_struct* deviceInfo)
{

}

__attribute__((weak))
void FullResetToFactory(base_device_information_struct* deviceInfo)
{
	//esp_partition_t* DataTable_partition = esp_partition_find_first(ESP_PARTITION_TYPE_ANY,ESP_PARTITION_SUBTYPE_ANY,"data_table");

	//	esp_partition_erase_range(DataTable_partition,0,DataTable_partition->size);


	Data_Table_Reset(deviceInfo->init_device_info->data_table);



//	DataTableInit(&data_table, partition_flash_memory_driver);

//	WriteDefaultInfo();

//	DataTableInitFinish(&data_table);
}



void init_base_device_info(init_device_information_struct* init_device_info,base_device_information_struct* device_info)
{

	if(device_info == 0) return;

	device_info->init_device_info = init_device_info;

	init_device_info->data_table = (data_table_struct*)malloc(sizeof(data_table_struct));
	memset(init_device_info->data_table, 0, sizeof(data_table_struct));

	init_device_info->data_table->DataTableStartAddres = init_device_info->DataTableStartAddres;
	init_device_info->data_table->default_data_table = init_device_info->default_data_table;

	DataTableInit(init_device_info->data_table, init_device_info->memory_driver);
	
	
	data_table_read_data_status_enum status;
	device_info->DeviceInfo = Data_Table_ReadData(init_device_info->data_table, DTCE_DEVICE_INFO_TYPE,&status);

}

void GetSerialNumber(command_selector_struct command)
{
	char* Serial = "SubModule CPU 12345";

//	MainAnswer(command.header_id, (uint8_t*)Serial, strlen(Serial),0,0);
}

